// Christmas Additions: Christmas in Isle Delfino, drawn by the mod, with no game code of its own.
//
// - Santa hats: every Pianta wears one, drawn each tick at its head joint (api->parts' "kubi").
// - Winter clothes: each Pianta's clothes ("fuku" materials) drawn in a winter colour.
// - Snowmen: every Strollin' Stu stack (Dango: TDangoHamuKuri, TBossDangoHamuKuri) is drawn white,
//   and its top ball wears a top hat, a scarf, coal eyes and mouth and a carrot nose; the balls
//   under it have coal buttons, the one under the head twig arms. A ball knocked off the top
//   leaves the next one down as the head.
//
// - Snow: in a level with an object named "Snowy ground", Mario's steps on open ground leave white
//   footprints (pressed snow) that fade, and running and landing kick up puffs of snow; where one is
//   named "Snowfall" (or everywhere, by choice) snow falls round the camera, out of doors. A level
//   gives its snow the Snow ground type (the world editor's), so the game leaves no sand prints.
// - Back through a level's door: leaving a stage by its own way out (a stage exit) and coming back,
//   Mario starts beside the exit that leads where he came from, not at the stage's start. A level
//   names its exits "Exit to <area> <episode>" (the world editor's do), so this works for any.
//
// The hats and snowman parts are models of the mod's own (api->models), placed at a joint of the
// actor's model (api->parts, Super Mario Sunshine Remastered's), so they follow its animations.
// Without api->parts the mod does nothing and says so in the log.
#include <sms_mod.hpp>

#include <math.h>
#include <stdio.h>
#include <string.h>

#include <set>
#include <vector>

namespace {

// ---------------------------------------------------------------- meshes with their light baked in

struct V {
	float x, y, z;
};
V v(float x, float y, float z)
{
	V r = {x, y, z};
	return r;
}
V add(V a, V b) { return v(a.x + b.x, a.y + b.y, a.z + b.z); }
V sub(V a, V b) { return v(a.x - b.x, a.y - b.y, a.z - b.z); }
V mul(V a, float s) { return v(a.x * s, a.y * s, a.z * s); }
float dot(V a, V b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
V cross(V a, V b) { return v(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x); }
V norm(V a)
{
	float l = sqrtf(dot(a, a));
	return l > 1e-6f ? mul(a, 1.0f / l) : v(1, 0, 0);
}

struct Rgb {
	uint8_t r, g, b;
};

// The parts' frame, both a Pianta's head ("kubi") and a Stu's body ("body"): x up, y forward, z to
// the side. The light comes from above, a little in front.
const float kPi = 3.14159265f;

struct Mesh {
	std::vector<SmsCorner> corners;
	V light;
	Mesh() : light(norm(v(0.75f, 0.5f, 0.3f))) {}

	void tri(V a, V b, V c, Rgb col)
	{
		V n       = norm(cross(sub(b, a), sub(c, a)));
		float lit = dot(n, light);
		float s   = 0.55f + 0.45f * (lit > 0 ? lit : 0.0f);
		SmsCorner k;
		k.r = (uint8_t)(col.r * s), k.g = (uint8_t)(col.g * s), k.b = (uint8_t)(col.b * s), k.a = 255;
		const V p[3] = {a, b, c};
		for (int i = 0; i < 3; i++) {
			k.x = p[i].x, k.y = p[i].y, k.z = p[i].z;
			corners.push_back(k);
		}
	}
	void quad(V a, V b, V c, V d, Rgb col) { tri(a, b, c, col), tri(a, c, d, col); }

	// a ring of `seg` points round `axis` at `c`
	static void ring(V c, V axis, float r, int seg, std::vector<V>& out)
	{
		V a = norm(axis);
		V u = norm(cross(a, fabsf(a.x) < 0.9f ? v(1, 0, 0) : v(0, 1, 0)));
		V w = cross(a, u);
		out.clear();
		for (int i = 0; i < seg; i++) {
			float t = 2 * kPi * i / seg;
			out.push_back(add(c, add(mul(u, r * cosf(t)), mul(w, r * sinf(t)))));
		}
	}
	// a tube along points `c` with radii `r` (a cone, a bent hat), capped at each end
	void tube(const std::vector<V>& c, const std::vector<float>& r, Rgb col, int seg = 16, bool caps = true)
	{
		std::vector<std::vector<V> > rings(c.size());
		for (size_t i = 0; i < c.size(); i++) {
			V axis = i + 1 < c.size() ? sub(c[i + 1], c[i]) : sub(c[i], c[i - 1]);
			ring(c[i], axis, r[i], seg, rings[i]);
		}
		for (size_t i = 0; i + 1 < c.size(); i++)
			for (int k = 0; k < seg; k++) {
				int k2 = (k + 1) % seg;
				quad(rings[i][k], rings[i][k2], rings[i + 1][k2], rings[i + 1][k], col);
			}
		if (caps) {
			for (int k = 0; k < seg; k++) {
				int k2 = (k + 1) % seg;
				tri(c[0], rings[0][k2], rings[0][k], col);
				tri(c.back(), rings.back()[k], rings.back()[k2], col);
			}
		}
	}
	void cylinder(V a, V b, float ra, float rb, Rgb col, int seg = 16)
	{
		std::vector<V> c(2);
		std::vector<float> r(2);
		c[0] = a, c[1] = b, r[0] = ra, r[1] = rb;
		tube(c, r, col, seg);
	}
	void sphere(V c, float r, Rgb col, int seg = 12, int rings = 8)
	{
		for (int i = 0; i < rings; i++) {
			float a0 = kPi * i / rings - kPi / 2, a1 = kPi * (i + 1) / rings - kPi / 2;
			for (int k = 0; k < seg; k++) {
				float b0 = 2 * kPi * k / seg, b1 = 2 * kPi * (k + 1) / seg;
				V p00 = add(c, mul(v(sinf(a0), cosf(a0) * cosf(b0), cosf(a0) * sinf(b0)), r));
				V p01 = add(c, mul(v(sinf(a0), cosf(a0) * cosf(b1), cosf(a0) * sinf(b1)), r));
				V p10 = add(c, mul(v(sinf(a1), cosf(a1) * cosf(b0), cosf(a1) * sinf(b0)), r));
				V p11 = add(c, mul(v(sinf(a1), cosf(a1) * cosf(b1), cosf(a1) * sinf(b1)), r));
				quad(p00, p01, p11, p10, col);
			}
		}
	}
	// a ring of fur or a scarf: round the x axis at height x, radius R, thickness r
	void torus(float x, float R, float r, Rgb col, int seg = 24, int side = 8)
	{
		for (int i = 0; i < seg; i++)
			for (int k = 0; k < side; k++) {
				V p[2][2];
				for (int di = 0; di < 2; di++)
					for (int dk = 0; dk < 2; dk++) {
						float a = 2 * kPi * (i + di) / seg, b = 2 * kPi * (k + dk) / side;
						float rr = R + r * cosf(b);
						p[di][dk] = v(x + r * sinf(b), rr * cosf(a), rr * sinf(a));
					}
				quad(p[0][0], p[1][0], p[1][1], p[0][1], col);
			}
	}
};

// A point on a ball (centre c, radius r): `up` and `side` degrees from straight ahead
V onBall(V c, float r, float up, float side)
{
	float e = up * kPi / 180, a = side * kPi / 180;
	return add(c, mul(v(sinf(e), cosf(e) * cosf(a), cosf(e) * sinf(a)), r));
}

const Rgb kWhite = {246, 246, 248}, kRed = {200, 22, 34}, kCoal = {30, 30, 34}, kCarrot = {245, 120, 28},
          kHat = {32, 32, 38}, kBand = {190, 24, 32}, kScarf = {205, 30, 40}, kTwig = {100, 64, 36};

// A Santa hat on a Pianta's head joint ("kubi": the game puts its own hats there, from 14 up)
void santaHat(Mesh& m)
{
	m.torus(20, 44, 10, kWhite);
	std::vector<V> c;
	std::vector<float> r;
	const int n = 10;
	for (int i = 0; i <= n; i++) {
		float t = (float)i / n;
		c.push_back(v(22 + 80 * t, -34 * t * t, 0)); // up, bending back
		r.push_back(42 * (1 - t) + 4 * t);
	}
	m.tube(c, r, kRed, 18);
	m.sphere(add(c.back(), v(0, -6, 0)), 13, kWhite);
}

// A Stu's ball in its body joint's frame (the game's model and the snowman level's alike)
const V kBall   = {49, 0, 0};
const float kR  = 63; // a little out from the ball, so nothing sinks into it

void snowmanHead(Mesh& m)
{
	m.sphere(onBall(kBall, kR, 24, -22), 7, kCoal);
	m.sphere(onBall(kBall, kR, 24, 22), 7, kCoal);
	for (int i = 0; i < 5; i++) { // a smile of coal
		float side = -24 + 12 * i;
		m.sphere(onBall(kBall, kR - 1, -14 + 5 * fabsf(side) / 24, side), 4, kCoal, 8, 6);
	}
	V base = onBall(kBall, kR - 6, 6, 0);
	m.cylinder(base, add(base, v(-3, 50, 0)), 10, 1, kCarrot, 12);
	// a top hat
	float x0 = kBall.x + kR * 0.8f;
	m.cylinder(v(x0, 0, 0), v(x0 + 5, 0, 0), 48, 48, kHat, 20);
	m.cylinder(v(x0 + 5, 0, 0), v(x0 + 60, 0, 0), 32, 33, kHat, 20);
	m.cylinder(v(x0 + 5, 0, 0), v(x0 + 15, 0, 0), 33.5f, 33.5f, kBand, 20);
	// a scarf where it meets the ball below, its end hanging at the front
	m.torus(kBall.x - kR * 0.72f, 46, 10, kScarf);
	V knot = v(kBall.x - kR * 0.72f, 40, 22);
	m.cylinder(knot, add(knot, v(-46, 10, 6)), 9, 8, kScarf, 8);
}

void snowmanButtons(Mesh& m)
{
	for (int i = 0; i < 3; i++)
		m.sphere(onBall(kBall, kR, 32 - 26 * i, 0), 8, kCoal);
}

void snowmanArms(Mesh& m)
{
	for (int s = -1; s <= 1; s += 2) {
		V a = onBall(kBall, kR - 8, 12, 90 * s);
		V b = add(a, v(42, 0, 64.0f * s));
		m.cylinder(a, b, 4, 3, kTwig, 6);
		V mid = add(a, mul(sub(b, a), 0.7f));
		m.cylinder(mid, add(mid, v(24, 6, 6.0f * s)), 3, 2, kTwig, 6);
	}
}

// Rudolph's antlers and nose, in a frame of the mod's own on Yoshi's head (rudolphFrame): x up, y the way
// his snout points (level), z to his side; the antlers' roots at the head joint, the nose at the mouth's
const Rgb kAntler = {156, 112, 70}, kAntlerTip = {196, 160, 112}, kNose = {236, 26, 30};
void antler(Mesh& m, float s)
{
	// a beam up from the head, curving back and out, with three tines forward
	std::vector<V> c;
	std::vector<float> r;
	const int n = 8;
	for (int i = 0; i <= n; i++) {
		float t = (float)i / n;
		c.push_back(v(30 + 78 * t, -6 - 30 * t + 16 * t * t, s * (24 + 34 * t)));
		r.push_back(7.5f * (1 - t) + 3 * t);
	}
	m.tube(c, r, kAntler, 10);
	const float at[3] = {0.35f, 0.62f, 0.88f}, len[3] = {30, 26, 20};
	for (int k = 0; k < 3; k++) {
		int i  = (int)(at[k] * n);
		V root = c[i];
		V tip  = add(root, v(len[k] * 0.8f, len[k] * 0.7f, s * len[k] * 0.15f));
		m.cylinder(root, tip, r[i] * 0.8f, 1.8f, k == 2 ? kAntlerTip : kAntler, 8);
	}
}
void rudolphAntlers(Mesh& m)
{
	antler(m, 1);
	antler(m, -1);
}
void rudolphNose(Mesh& m)
{
	m.light = norm(v(0.4f, 0.9f, 0.2f));
	m.sphere(v(0, 0, 0), 24, kNose, 16, 10);
	m.sphere(onBall(v(0, 0, 0), 17, 40, -25), 6, Rgb{255, 190, 190}, 8, 6); // a shine
}

// Names as the game keeps them (Shift JIS)
const char kDango[] = "\x82\xbe\x82\xf1\x82\xb2\x83\x6e\x83\x80\x83\x4e\x83\x8a"; // だんごハムクリ
const char kMonte[] = "\x83\x82\x83\x93\x83\x65";                                 // モンテ (a Pianta)

// Winter colours for Piantas' clothes, picked by each one's place in the stage
const SmsColor kWinter[] = {0xB81E2AE0u, 0x1E7840E0u, 0x23408CE0u, 0xE8E8EEE0u, 0x7A1830E0u, 0x2F5A2AE0u};

struct Ball {
	SmsObject obj;
	float m[3][4];
	V centre;
	float radius;
};

// ---------------------------------------------------------------- snow: prints, puffs and flakes

// Pictures: a boot pressed into snow, and soft round dots.
// The print is one boot shape (wide toe, narrower arch, round heel), seen as a dip: its floor
// a little shaded, its walls darker, the toe pressed deepest, a faint lip of snow pushed up
// round it, and some grain so it is not a clean sticker.
const int kPrintW = 64, kPrintH = 128;
float smooth01(float a, float b, float x)
{
	float t = (x - a) / (b - a);
	t = t < 0 ? 0 : (t > 1 ? 1 : t);
	return t * t * (3 - 2 * t);
}
float grain(int x, int y)
{
	unsigned h = (unsigned)x * 374761393u + (unsigned)y * 668265263u;
	h = (h ^ (h >> 13)) * 1274126177u;
	return ((h ^ (h >> 16)) & 1023) / 1023.0f; // 0..1
}
std::vector<uint8_t> printPicture()
{
	const int w = kPrintW, h = kPrintH;
	std::vector<uint8_t> px(w * h * 4);
	for (int y = 0; y < h; y++)
		for (int x = 0; x < w; x++) {
			float u = (x + 0.5f) / w * 2 - 1, vv = (y + 0.5f) / h * 2 - 1; // -1..1, the toe at the top
			// half the boot's width along it: toe 0.78, arch 0.52, heel 0.62
			float hw = 0.78f - 0.26f * expf(-((vv - 0.30f) * (vv - 0.30f)) / 0.05f) - 0.16f * smooth01(0.45f, 0.95f, vv);
			float au = fabsf(u) / (hw * 0.80f), av = fabsf(vv) / 0.80f;
			float d = powf(powf(au, 2.6f) + powf(av, 2.6f), 1 / 2.6f); // under 1 inside the boot
			d += (grain(x / 5, y / 5) - 0.5f) * 0.06f;                  // a ragged edge
			float g = grain(x, y) - 0.5f;
			float r, gr, b, a;
			if (d < 1) {
				float wall = smooth01(0.62f, 1.0f, d);                 // 0 on the floor, 1 at the wall
				float toe  = smooth01(0.2f, -0.7f, vv);                // the toe pressed deeper
				float dark = 0.30f + 0.45f * wall + 0.15f * toe;       // how dark: shade of the dip
				r = 150 - 60 * dark, gr = 164 - 58 * dark, b = 205 - 40 * dark;
				a = (0.42f + 0.33f * wall + 0.12f * toe) * (1 + g * 0.35f);
			} else {
				float lip = 1 - smooth01(1.0f, 1.28f, d);              // pushed-up snow round it
				r = 255, gr = 255, b = 255;
				a = 0.30f * lip * (1 + g * 0.5f);
			}
			a = a < 0 ? 0 : (a > 1 ? 1 : a);
			uint8_t* p = &px[(y * w + x) * 4];
			p[0] = (uint8_t)r, p[1] = (uint8_t)gr, p[2] = (uint8_t)b, p[3] = (uint8_t)(a * 255);
		}
	return px;
}

std::vector<uint8_t> softDot(int w, float hard)
{
	std::vector<uint8_t> px(w * w * 4);
	for (int y = 0; y < w; y++)
		for (int x = 0; x < w; x++) {
			float u = (x + 0.5f) / w * 2 - 1, vv = (y + 0.5f) / w * 2 - 1;
			float r = sqrtf(u * u + vv * vv), a = r < hard ? 1.0f : (r < 1 ? (1 - r) / (1 - hard) : 0.0f);
			uint8_t* p = &px[(y * w + x) * 4];
			p[0] = 255, p[1] = 255, p[2] = 255, p[3] = (uint8_t)(a * a * 255);
		}
	return px;
}

struct Print {
	V p;
	float yaw, born, size;
};
struct Puff {
	V p;
	float born, size;
};
struct Flake {
	V at; // where it starts in the box round the camera
	float fall, sway, size, phase;
};

// a quad, corners a b c d (the picture's 0,0 1,0 1,1 0,1), one colour
void quad(std::vector<SmsTexCorner>& out, V a, V b, V c, V d, uint8_t r, uint8_t g, uint8_t bl, uint8_t al)
{
	const V p[6]         = {a, b, c, a, c, d};
	const float uv[6][2] = {{0, 0}, {1, 0}, {1, 1}, {0, 0}, {1, 1}, {0, 1}};
	for (int i = 0; i < 6; i++) {
		SmsTexCorner k;
		k.x = p[i].x, k.y = p[i].y, k.z = p[i].z, k.u = uv[i][0], k.v = uv[i][1];
		k.r = r, k.g = g, k.b = bl, k.a = al;
		out.push_back(k);
	}
}

// ---------------------------------------------------------------- the mod

struct ChristmasAdditions : sms::Mod {
	using sms::Mod::Mod;

	const SmsPartsApi* parts = nullptr;
	SmsMesh hat = 0, head = 0, buttons = 0, arms = 0;
	SmsMesh antlers = 0, nose = 0;
	bool rudolphOn = false, yoshiLogged = false; // Yoshi's colour set; his joints logged
	char yoshiHead[32];
	std::set<SmsObject> dressed, snowy; // Piantas' clothes and snowmen tinted, this stage
	int leftArea = -1, leftEpisode = -1; // the stage Mario left by its way out, till he starts in the next

	// ---------------------------------------------------------------- snow
	SmsTexture printTex = 0, puffTex = 0, flakeTex = 0;
	bool snowyGround = false, snowfall = false; // this stage's markers
	int markerLooks = 0;                         // looked for them this many times this stage
	float now = 0;
	std::vector<Print> prints;
	std::vector<Puff> puffs;
	std::vector<Flake> flakes;
	V last;
	bool lastValid = false, wasOnGround = true;
	float stride = 0, lastVy = 0, snowLevel = 0;
	int side = 1;
	unsigned printSeed = 777;

	void addPuff(V p, float size)
	{
		if (puffs.size() >= 96)
			puffs.erase(puffs.begin());
		Puff f = {p, now, size};
		puffs.push_back(f);
	}

	// open ground under a point: ordinary floor (not ice, water and the like) with no roof over it
	bool openGround(const SmsVec3& at, float* floorY)
	{
		SmsVec3 probe = at;
		probe.y += 60;
		SmsGround g;
		memset(&g, 0, sizeof g);
		g.size = sizeof g;
		if (!game().ground || !game().ground(&probe, &g))
			return false;
		if (floorY)
			*floorY = g.floor;
		return (g.floor_type & 0x0FFF) == 0 && !g.water && g.roof > at.y + 2500;
	}
	bool outdoors(const SmsVec3& at)
	{
		SmsVec3 probe = at;
		probe.y += 60;
		SmsGround g;
		memset(&g, 0, sizeof g);
		g.size = sizeof g;
		return !game().ground || !game().ground(&probe, &g) || g.roof > at.y + 2500;
	}

	// Mario's steps on snowy open ground: a print each stride, puffs as he runs and lands
	void steps(const SmsPlayer& p, float dt)
	{
		V at        = v(p.position.x, p.position.y, p.position.z);
		float moved = lastValid ? sqrtf((at.x - last.x) * (at.x - last.x) + (at.z - last.z) * (at.z - last.z)) : 0;
		if (moved > 300)
			moved = 0; // a warp, not a step
		last = at, lastValid = true;
		bool landed = p.on_ground && !wasOnGround && lastVy < -8;
		wasOnGround = p.on_ground != 0;
		lastVy      = p.velocity.y;
		float floorY;
		if (!p.on_ground || p.in_water || p.on_yoshi || !openGround(p.position, &floorY))
			return;
		const float yaw = p.yaw * kPi / 180;
		const V fwd = v(sinf(yaw), 0, cosf(yaw)), right = v(cosf(yaw), 0, -sinf(yaw));
		const V ground = v(at.x, floorY + 1.5f, at.z);
		if (landed)
			for (int i = 0; i < 6; i++) {
				float a = 2 * kPi * i / 6;
				addPuff(add(ground, v(cosf(a) * 30, 4, sinf(a) * 30)), 34);
			}
		stride += moved;
		const float speed = dt > 0 ? moved / dt : 0; // units a second
		// a print a step: longer steps the faster he goes
		float step = 70 + speed * 0.06f;
		step = step > 125 ? 125 : step;
		if (stride > step) {
			stride = 0;
			side   = -side;
			printSeed = printSeed * 1103515245u + 12345u;
			float jitter = ((printSeed >> 8) % 1000) / 1000.0f - 0.5f; // -0.5..0.5
			V foot   = add(ground, mul(right, 14.0f * side));
			Print pr = {foot, yaw + jitter * 0.16f + side * 0.06f, now, 1 + jitter * 0.12f};
			if (prints.size() >= 160)
				prints.erase(prints.begin());
			prints.push_back(pr);
			if (speed > 500)
				addPuff(add(foot, mul(fwd, -12)), 26);
		}
	}

	void drawSnow(const SmsCamera& cam)
	{
		std::vector<SmsTexCorner> c;
		// prints: fading out over their last two seconds of fourteen
		const float life = 14;
		while (!prints.empty() && now - prints[0].born > life)
			prints.erase(prints.begin());
		for (size_t i = 0; i < prints.size(); i++) {
			const Print& pr = prints[i];
			float age = (now - pr.born) / life, a = age < 0.85f ? 1.0f : (1 - age) / 0.15f;
			V f = v(sinf(pr.yaw), 0, cosf(pr.yaw)), r = v(cosf(pr.yaw), 0, -sinf(pr.yaw));
			V hf = mul(f, 30 * pr.size), hr = mul(r, 15 * pr.size); // a boot about 60 long, Mario's size
			quad(c, add(sub(pr.p, hr), hf), add(add(pr.p, hr), hf), sub(add(pr.p, hr), hf), sub(sub(pr.p, hr), hf), 255, 255, 255,
			     (uint8_t)(255 * a));
		}
		if (!c.empty())
			api().models->draw_textured(&c[0], (int)c.size(), printTex, SMS_BLEND_ALPHA);
		// puffs and flakes face the camera
		V right = norm(v(cam.view[0][0], cam.view[0][1], cam.view[0][2]));
		V up    = norm(v(cam.view[1][0], cam.view[1][1], cam.view[1][2]));
		c.clear();
		while (!puffs.empty() && now - puffs[0].born > 0.8f)
			puffs.erase(puffs.begin());
		for (size_t i = 0; i < puffs.size(); i++) {
			const Puff& pf = puffs[i];
			float t = (now - pf.born) / 0.8f, size = pf.size * (0.6f + 1.4f * t);
			V m = add(pf.p, v(0, 40 * t, 0)), hr = mul(right, size), hu = mul(up, size);
			quad(c, add(sub(m, hr), hu), add(add(m, hr), hu), sub(add(m, hr), hu), sub(sub(m, hr), hu), 255, 255, 255,
			     (uint8_t)(200 * (1 - t)));
		}
		if (!c.empty())
			api().models->draw_textured(&c[0], (int)c.size(), puffTex, SMS_BLEND_ALPHA);
		// flakes: a box of them round the camera, falling and swaying, each wrapping round inside it
		if (snowLevel <= 0.01f || flakes.empty())
			return;
		c.clear();
		const float W = 3600, H = 2200;
		V cp = v(cam.position.x, cam.position.y, cam.position.z);
		for (size_t i = 0; i < flakes.size(); i++) {
			const Flake& fl = flakes[i];
			V p = v(fl.at.x + 40 * now + sinf(now * 0.9f + fl.phase) * fl.sway, fl.at.y - fl.fall * now, fl.at.z + 18 * now);
			p.x  = cp.x + fmodf(fmodf(p.x - cp.x, W) + W * 1.5f, W) - W / 2;
			p.y  = cp.y + fmodf(fmodf(p.y - cp.y, H) + H * 1.5f, H) - H / 2;
			p.z  = cp.z + fmodf(fmodf(p.z - cp.z, W) + W * 1.5f, W) - W / 2;
			V hr = mul(right, fl.size), hu = mul(up, fl.size);
			quad(c, add(sub(p, hr), hu), add(add(p, hr), hu), sub(add(p, hr), hu), sub(sub(p, hr), hu), 255, 255, 255,
			     (uint8_t)(235 * snowLevel));
		}
		api().models->draw_textured(&c[0], (int)c.size(), flakeTex, SMS_BLEND_ALPHA);
	}

	void lookForMarkers()
	{
		if (markerLooks > 8 || !game().find_object)
			return;
		markerLooks++;
		snowyGround = game().find_object("Snowy ground") != nullptr;
		snowfall    = game().find_object("Snowfall") != nullptr;
		if (snowyGround || snowfall)
			markerLooks = 99; // found: done for this stage
	}

	void snow(float dt)
	{
		now += dt;
		lookForMarkers();
		const bool stepsOn = config().get_int(handle(), "snow_steps", 1) != 0;
		const int fall     = config().get_int(handle(), "snowfall", 1); // 0 off, 1 snowy levels, 2 everywhere
		SmsPlayer p;
		memset(&p, 0, sizeof p);
		p.size = sizeof p;
		if (!player().get || !player().get(&p) || !p.valid)
			return;
		if (stepsOn && snowyGround)
			steps(p, dt);
		// snow falls out of doors: thinning away under a roof, over a second
		bool want = (fall == 2 || (fall == 1 && snowfall)) && outdoors(p.position);
		snowLevel += want ? dt : -dt;
		snowLevel = snowLevel < 0 ? 0 : (snowLevel > 1 ? 1 : snowLevel);
		if (prints.empty() && puffs.empty() && snowLevel <= 0)
			return;
		SmsCamera cam;
		memset(&cam, 0, sizeof cam);
		cam.size = sizeof cam;
		if (!SMS_API_HAS(&api(), camera) || !api().camera->get(&cam) || !cam.valid)
			return;
		drawSnow(cam);
	}

	void makeSnow()
	{
		std::vector<uint8_t> px = printPicture();
		printTex                = api().models->texture_create(handle(), &px[0], kPrintW, kPrintH);
		px                      = softDot(32, 0.15f);
		puffTex                 = api().models->texture_create(handle(), &px[0], 32, 32);
		px                      = softDot(16, 0.45f);
		flakeTex                = api().models->texture_create(handle(), &px[0], 16, 16);
		unsigned seed           = 12345;
		for (int i = 0; i < 1100; i++) {
			Flake f;
			seed = seed * 1103515245u + 12345u, f.at.x = (seed >> 8) % 3600;
			seed = seed * 1103515245u + 12345u, f.at.y = (seed >> 8) % 2200;
			seed = seed * 1103515245u + 12345u, f.at.z = (seed >> 8) % 3600;
			seed = seed * 1103515245u + 12345u, f.fall = 110 + (seed >> 8) % 70;
			seed = seed * 1103515245u + 12345u, f.sway = 10 + (seed >> 8) % 30;
			seed = seed * 1103515245u + 12345u, f.size = 3.5f + ((seed >> 8) % 40) / 10.0f;
			seed = seed * 1103515245u + 12345u, f.phase = ((seed >> 8) % 628) / 100.0f;
			flakes.push_back(f);
		}
	}

	// Mario starting where he came back to: beside the exit that leads to the stage he left, facing
	// the way it faces, a little in front of it (so he isn't sent straight back)
	void spawnBesideExit(SmsEvPlayerSpawn& sp)
	{
		if (leftArea < 0)
			return;
		char name[48];
		snprintf(name, sizeof name, "Exit to %d %d", leftArea, leftEpisode);
		leftArea = leftEpisode = -1;
		SmsObject exit = game().find_object ? game().find_object(name) : nullptr;
		SmsVec3 at;
		if (!exit || !game().object_position(exit, &at))
			return;
		// its turn and size: the stage's exit object at that place
		float yaw = 0, size = 1;
		SmsObjectInfo oi;
		for (int i = 0;; i++) {
			memset(&oi, 0, sizeof oi);
			oi.size = sizeof oi;
			if (!game().object_info || !game().object_info(0, 0xFFFFFFFFu, "", i, &oi))
				break;
			float dx = oi.position.x - at.x, dy = oi.position.y - at.y, dz = oi.position.z - at.z;
			if (dx * dx + dy * dy + dz * dz < 1.0f) {
				yaw = oi.rotation.y, size = oi.scale.x > 0 ? oi.scale.x : 1;
				break;
			}
		}
		const float out = 100.0f * size + 180.0f, a = yaw * kPi / 180.0f;
		sp.position.x = at.x + sinf(a) * out;
		sp.position.z = at.z + cosf(a) * out;
		// on the floor there (an exit down a hole sits below it): the floor under a point well above
		sp.position.y = at.y;
		SmsVec3 probe = sp.position;
		probe.y      = at.y + 800.0f;
		SmsGround g;
		memset(&g, 0, sizeof g);
		g.size = sizeof g;
		if (game().ground && game().ground(&probe, &g) && g.floor > at.y - 200.0f)
			sp.position.y = g.floor;
		sp.yaw          = yaw;
		sp.has_position = 1;
		info("back beside %s", name);
	}

	SmsMesh make(void (*build)(Mesh&))
	{
		Mesh m;
		build(m);
		return api().models->create(handle(), &m.corners[0], (int)m.corners.size());
	}

	bool named(SmsObject o, const char* part)
	{
		const char* n = game().object_name ? game().object_name(o) : nullptr;
		return n && strstr(n, part);
	}

	void draw(SmsMesh mesh, const float m[3][4])
	{
		if (mesh)
			api().models->draw(mesh, SMS_FRAME_WORLD, 0, m);
	}

	void untint(std::set<SmsObject>& set, bool parts_)
	{
		for (std::set<SmsObject>::iterator i = set.begin(); i != set.end(); ++i) {
			if (parts_)
				parts->tint(*i, nullptr, 0);
			else if (game().object_set_tint)
				game().object_set_tint(*i, 0);
		}
		set.clear();
	}

	bool partsHave(size_t end) { return parts && parts->size >= end; }

	// Yoshi as Rudolph: reindeer brown whatever fruit he ate, antlers, and a glowing red nose
	void rudolph(const SmsCamera* cam)
	{
		if (!partsHave(offsetof(SmsPartsApi, yoshi_color) + sizeof(void*)))
			return;
		const bool want = config().get_int(handle(), "rudolph", 1) != 0;
		if (want != rudolphOn) {
			parts->yoshi_color(want ? 0x7A4E2EFFu : 0u);
			rudolphOn = want;
		}
		SmsObject y = want ? parts->player_part(SMS_PARTS_YOSHI) : nullptr;
		if (!y)
			return;
		if (!yoshiLogged) {
			// his joints, once: a joint with "head" in its name carries the antlers, else his neck's top
			yoshiLogged   = true;
			yoshiHead[0]  = 0;
			int count     = parts->joint_count(y);
			for (int i = 0; i < count; i++) {
				const char* n = parts->joint_name(y, i);
				float jm[3][4];
				if (n && parts->joint(y, n, jm))
					info("yoshi joint %d %s at %.0f %.0f %.0f", i, n, jm[0][3], jm[1][3], jm[2][3]);
				if (n && !yoshiHead[0] && (strstr(n, "head") || strstr(n, "Head")))
					snprintf(yoshiHead, sizeof yoshiHead, "%s", n);
			}
			if (!yoshiHead[0])
				snprintf(yoshiHead, sizeof yoshiHead, "%s", "jnt_neck_2");
			info("rudolph: antlers on %s", yoshiHead);
		}
		float hm[3][4], mm[3][4];
		if (!parts->joint(y, yoshiHead, hm) || !parts->joint(y, "jnt_mouth", mm))
			return;
		V headAt = v(hm[0][3], hm[1][3], hm[2][3]), mouth = v(mm[0][3], mm[1][3], mm[2][3]);
		V fwd = sub(mouth, headAt);
		fwd.y = 0;
		if (dot(fwd, fwd) < 1)
			return;
		fwd = norm(fwd);
		V up = v(0, 1, 0), side = cross(fwd, up);
		// columns: x up, y forward, z side
		float f[3][4];
		const V axes[3] = {up, fwd, side};
		for (int r = 0; r < 3; r++)
			for (int k = 0; k < 3; k++)
				f[r][k] = (&axes[k].x)[r];
		f[0][3] = headAt.x, f[1][3] = headAt.y, f[2][3] = headAt.z;
		draw(antlers, f);
		// the nose: on the front of his snout, over the mouth
		V noseAt = add(add(mouth, mul(fwd, 26)), mul(up, 30));
		f[0][3] = noseAt.x, f[1][3] = noseAt.y, f[2][3] = noseAt.z;
		draw(nose, f);
		// its glow, facing the camera, pulsing a little
		if (cam && puffTex) {
			V right = norm(v(cam->view[0][0], cam->view[0][1], cam->view[0][2]));
			V camUp = norm(v(cam->view[1][0], cam->view[1][1], cam->view[1][2]));
			float g = 70 + 8 * sinf(now * 3.0f);
			V hr = mul(right, g), hu = mul(camUp, g);
			std::vector<SmsTexCorner> c;
			quad(c, add(sub(noseAt, hr), hu), add(add(noseAt, hr), hu), sub(add(noseAt, hr), hu), sub(sub(noseAt, hr), hu), 255, 40, 30, 150);
			api().models->draw_textured(&c[0], (int)c.size(), puffTex, SMS_BLEND_ADD);
		}
	}

	void tick()
	{
		{
			SmsCamera cam;
			memset(&cam, 0, sizeof cam);
			cam.size = sizeof cam;
			bool camOk = SMS_API_HAS(&api(), camera) && api().camera->get(&cam) && cam.valid;
			rudolph(camOk ? &cam : nullptr);
		}
		const bool hats = config().get_int(handle(), "santa_hats", 1) != 0;
		const bool clothes = config().get_int(handle(), "winter_clothes", 1) != 0;
		const bool snowmen = config().get_int(handle(), "snowmen", 1) != 0;
		if (!clothes && !dressed.empty())
			untint(dressed, true); // switched off
		if (!snowmen && !snowy.empty())
			untint(snowy, false);
		SmsPlayer p;
		memset(&p, 0, sizeof p);
		p.size = sizeof p;
		if (!player().get || !player().get(&p) || !p.valid)
			return;
		static SmsObject found[160];
		// Piantas
		if (hats || clothes) {
			int n = api().actor->near(&p.position, 9000.0f, SMS_ACTOR_FLAG_CHARACTER, found, 160);
			if (n > 160)
				n = 160;
			for (int i = 0; i < n; i++) {
				SmsObject o = found[i];
				float m[3][4];
				if (!named(o, kMonte) || !parts->joint(o, "kubi", m))
					continue;
				if (hats)
					draw(hat, m);
				if (clothes && !dressed.count(o)) {
					unsigned pick = (unsigned)(m[0][3] * 0.37f + m[2][3] * 0.61f);
					parts->tint(o, "fuku", kWinter[pick % (sizeof kWinter / sizeof kWinter[0])]);
					dressed.insert(o);
				}
			}
		}
		// snowmen
		if (snowmen) {
			int n = api().actor->near(&p.position, 9000.0f, SMS_ACTOR_FLAG_SMALL_ENEMY, found, 160);
			if (n > 160)
				n = 160;
			std::vector<Ball> balls;
			for (int i = 0; i < n; i++) {
				Ball b;
				b.obj = found[i];
				if (!named(b.obj, kDango) || !parts->joint(b.obj, "body", b.m))
					continue;
				V up     = v(b.m[0][0], b.m[1][0], b.m[2][0]); // the joint's x axis, scaled as the model is
				float sc = sqrtf(dot(up, up));
				b.radius = 60 * sc;
				b.centre = add(v(b.m[0][3], b.m[1][3], b.m[2][3]), mul(up, kBall.x));
				balls.push_back(b);
				if (!snowy.count(b.obj) && game().object_set_tint) {
					game().object_set_tint(b.obj, 0xF4F7FFFFu);
					snowy.insert(b.obj);
				}
			}
			for (size_t i = 0; i < balls.size(); i++) {
				const Ball& b = balls[i];
				bool above = false, belowHead = false;
				for (size_t k = 0; k < balls.size(); k++) {
					if (k == i)
						continue;
					const Ball& o = balls[k];
					float dx = o.centre.x - b.centre.x, dz = o.centre.z - b.centre.z, dy = o.centre.y - b.centre.y;
					if (dx * dx + dz * dz > b.radius * b.radius * 0.6f || dy < b.radius * 0.8f || dy > b.radius * 3.2f)
						continue;
					above = true;
					// the one on top of it has nothing over it: then this one holds the head up
					bool topOther = true;
					for (size_t j = 0; j < balls.size(); j++) {
						if (j == k)
							continue;
						const Ball& q = balls[j];
						float qx = q.centre.x - o.centre.x, qz = q.centre.z - o.centre.z, qy = q.centre.y - o.centre.y;
						if (qx * qx + qz * qz <= o.radius * o.radius * 0.6f && qy >= o.radius * 0.8f && qy <= o.radius * 3.2f)
							topOther = false;
					}
					if (topOther)
						belowHead = true;
				}
				if (!above)
					draw(head, b.m);
				else {
					draw(buttons, b.m);
					if (belowHead)
						draw(arms, b.m);
				}
			}
		}
	}

	int load() override
	{
		optionBool("santa_hats", "Santa hats on Piantas", true);
		optionBool("winter_clothes", "Winter colours on Piantas' clothes", true);
		optionBool("snowmen", "Strollin' Stu stacks are snowmen", true);
		optionBool("exit_return", "Back through a level's door, start beside it", true);
		optionBool("snow_steps", "White footprints and puffs of snow in snowy levels", true);
		optionChoice("snowfall", "Falling snow", {"Off", "In snowy levels", "Everywhere"}, 1);
		optionBool("rudolph", "Yoshi is Rudolph the red-nosed reindeer", true);

		// snow: the stage's markers looked for again, its prints and puffs gone with it
		on(SMS_EV_STAGE_LOADED, [this](SmsEvent&) {
			markerLooks = 0, snowyGround = snowfall = false;
			prints.clear(), puffs.clear(), lastValid = false;
		});
		if (SMS_API_HAS(&api(), models)) {
			makeSnow();
			on<SmsEvTick>(SMS_EV_TICK, [this](SmsEvTick& t) { snow(t.dt > 0 ? t.dt : 1.0f / 60); });
		}
		// a stage left by its own way out (a stage exit): Mario starts beside the exit back to it
		on<SmsEvStage>(SMS_EV_STAGE_LEAVING, [this](SmsEvStage& st) {
			SmsGameState g;
			memset(&g, 0, sizeof g);
			g.size = sizeof g;
			game().state(&g);
			const bool wayOut = st.reason == SMS_LEAVE_WAY_OUT;
			leftArea    = wayOut ? g.area : -1;
			leftEpisode = wayOut ? g.episode : -1;
		});
		on<SmsEvPlayerSpawn>(SMS_EV_PLAYER_SPAWN, [this](SmsEvPlayerSpawn& sp) {
			if (config().get_int(handle(), "exit_return", 1))
				spawnBesideExit(sp);
			else
				leftArea = leftEpisode = -1;
		});
		if (SMS_API_HAS(&api(), parts) && api().parts->magic == SMS_PARTS_MAGIC)
			parts = api().parts;
		if (!parts || !SMS_API_HAS(&api(), models) || !SMS_API_HAS(&api(), actor)) {
			warn("this game has no api->parts (Super Mario Sunshine Remastered's): no hats, clothes or snowmen");
			return 0;
		}
		antlers = make(rudolphAntlers);
		nose    = make(rudolphNose);
		hat     = make(santaHat);
		head    = make(snowmanHead);
		buttons = make(snowmanButtons);
		arms    = make(snowmanArms);

		on(SMS_EV_STAGE_LOADED, [this](SmsEvent&) { dressed.clear(), snowy.clear(); }); // the tints went with the old stage
		on(SMS_EV_TICK, [this](SmsEvent&) { tick(); });
		info("loaded: Santa hats, winter clothes, snowmen and Rudolph");
		return 0;
	}
};

} // namespace

SMS_MOD_CLASS(ChristmasAdditions)
