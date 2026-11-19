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

// Rudolph's antlers, ears and nose. Made in Yoshi's model's own rest pose (yoshi_model.bmd: x his left,
// y up, z forward; feet at 0, the top of his head at about 168, the tip of his snout at z 63, its nose
// point null_nose at 0 144 56), then put into his head joint's frame ("jnt_head", at 0 101 0 at rest:
// its x forward, y down, z his left), so they move with his head as the game poses it.
const Rgb kAntler = {150, 108, 66}, kAntlerTip = {204, 170, 120}, kNose = {236, 26, 30}, kEar = {122, 78, 46},
          kEarIn = {214, 168, 132};
const V kHeadAt = {-0.0000316f, 101.3156f, -0.0088f};
const V kHeadX = {-0.0000959f, -0.0185984f, 0.9998270f}, kHeadY = {0.0000018f, -0.9998270f, -0.0185984f},
        kHeadZ = {1.0f, 0.0f, 0.0000959f};
const V kNoseAt = {0, 143, 61}; // the red nose's middle, on the tip of his snout
V restToHead(V p)
{
	V rel = sub(p, kHeadAt);
	return v(dot(rel, kHeadX), dot(rel, kHeadY), dot(rel, kHeadZ));
}
void intoHead(Mesh& m)
{
	for (size_t i = 0; i < m.corners.size(); i++) {
		V q = restToHead(v(m.corners[i].x, m.corners[i].y, m.corners[i].z));
		m.corners[i].x = q.x, m.corners[i].y = q.y, m.corners[i].z = q.z;
	}
}
void antler(Mesh& m, float s)
{
	// a beam from the top of his head behind the eyes, up, out and a little back, three tines forward
	std::vector<V> c;
	std::vector<float> r;
	const int n = 8;
	for (int i = 0; i <= n; i++) {
		float t = (float)i / n;
		c.push_back(v(s * (13 + 30 * t), 158 + 58 * t - 6 * t * t, -4 - 16 * t + 8 * t * t));
		r.push_back(4.6f * (1 - t) + 1.8f * t);
	}
	m.tube(c, r, kAntler, 10);
	const float at[3] = {0.3f, 0.58f, 0.85f}, len[3] = {20, 18, 13};
	for (int k = 0; k < 3; k++) {
		int i  = (int)(at[k] * n + 0.5f);
		V root = c[i];
		V tip  = add(root, v(s * len[k] * 0.15f, len[k] * 0.65f, len[k] * 0.75f));
		m.cylinder(root, tip, r[i] * 0.75f, 1.2f, k == 2 ? kAntlerTip : kAntler, 8);
	}
}
void ear(Mesh& m, float s)
{
	// a leaf-shaped ear out from the side of his head, below the antler (the head is about 20 wide there)
	std::vector<V> c;
	std::vector<float> r;
	for (int i = 0; i <= 5; i++) {
		float t = i / 5.0f;
		c.push_back(v(s * (16 + 19 * t), 148 + 5 * t, 1 - 7 * t)); // its root sunk into the side of his head
		r.push_back(6.5f * sinf(kPi * (0.15f + 0.85f * t)) + 0.6f);
	}
	m.tube(c, r, kEar, 10);
}
void rudolphAntlers(Mesh& m)
{
	m.light = norm(v(0.2f, 0.9f, 0.4f));
	antler(m, 1);
	antler(m, -1);
	ear(m, 1);
	ear(m, -1);
	intoHead(m);
}
void rudolphNose(Mesh& m)
{
	m.light = norm(v(0.2f, 0.9f, 0.4f));
	m.sphere(kNoseAt, 13, kNose, 16, 10);
	intoHead(m);
}
// A little snow fallen on top of his nose (the "rudolph_nose_snow" option; Allebale asked for less): a
// small, thin patch on its very top, its edge ragged, a touch blue at the edge, and two small clumps
void rudolphNoseSnow(Mesh& m)
{
	m.light = norm(v(0.2f, 0.9f, 0.4f));
	const int seg = 28, rings = 6;
	const Rgb snow = {246, 248, 255}, shade = {214, 224, 240};
	std::vector<std::vector<V> > ring(rings + 1);
	for (int i = 0; i <= rings; i++)
		for (int k = 0; k < seg; k++) {
			float az   = 2 * kPi * k / seg;
			// the edge comes down further at the back and the sides than at the front, and wavers
			float edge = 0.98f + 0.10f * cosf(az) + 0.06f * sinf(3 * az + 1.3f) + 0.04f * sinf(7 * az);
			float t    = (float)i / rings;                      // 0 the top, 1 the edge
			float el   = kPi / 2 - t * (kPi / 2 - edge);        // elevation over the nose's middle
			float lump = 0.5f + 0.35f * (1 - t) * (0.6f + 0.4f * sinf(5 * az + 2 * i)); // thickest on top
			float r    = 13.4f + lump * (1 - t * 0.8f);
			ring[i].push_back(add(kNoseAt, v(r * cosf(el) * sinf(az), r * sinf(el), r * cosf(el) * cosf(az))));
		}
	for (int i = 0; i < rings; i++)
		for (int k = 0; k < seg; k++) {
			int k2 = (k + 1) % seg;
			m.quad(ring[i][k], ring[i + 1][k], ring[i + 1][k2], ring[i][k2], i == rings - 1 ? shade : snow);
		}
	for (int k = 0; k < 2; k++) { // clumps
		float az = 0.8f + 2.6f * k, el = 1.3f + 0.1f * k;
		V at = add(kNoseAt, v(13.9f * cosf(el) * sinf(az), 13.9f * sinf(el), 13.9f * cosf(el) * cosf(az)));
		m.sphere(at, 1.2f + 0.4f * k, snow, 8, 5);
	}
	intoHead(m);
}

// ---------------------------------------------------------------- Santa Mario and Christmas FLUDD
// Made in the models' own rest poses (mario.szs: bmd/ma_mdl1.bmd, Mario standing with his arms out, feet at
// 0, x his left, y up, z forward; watergun2/body/wg_mdl1.bmd, FLUDD), then carried into the frame of the
// joint each hangs on, as the game poses it. A joint's rest frame: where it is, and its x, y, z axes.
struct RestFrame {
	V at, x, y, z;
};
const RestFrame kMarioHead  = {{0, 97, -3.004f}, {-0.0001f, 0.0002f, 1}, {0, -1, 0.0002f}, {1, 0, 0.0001f}};
const RestFrame kMarioWaist = {{0, 58, 0}, {0.0001f, -1, 0}, {0.0001f, 0, -1}, {1, 0.0001f, 0.0001f}};
const RestFrame kMarioArmR  = {{-29.425f, 80.107f, -5.553f}, {-0.9675f, 0, 0.253f}, {-0.253f, 0.0002f, -0.9675f}, {0, -1, -0.0002f}};
const RestFrame kMarioArmL  = {{29.426f, 80.107f, -5.548f}, {0.9675f, 0.0001f, 0.253f}, {0.253f, 0.0001f, -0.9675f}, {-0.0001f, 1, 0.0001f}};
const RestFrame kMarioLegR  = {{-11.277f, 29.19f, -1.52f}, {0.0001f, -0.9973f, -0.0733f}, {0.0001f, 0.0733f, -0.9973f}, {1, 0.0001f, 0.0001f}};
const RestFrame kMarioLegL  = {{11.283f, 29.192f, -1.518f}, {0.0001f, -0.9973f, -0.0733f}, {0.0001f, 0.0733f, -0.9973f}, {1, 0.0001f, 0.0001f}};
const RestFrame kFluddMain  = {{0, 0, 0}, {0.0002f, -1, 0}, {1, 0.0002f, -0.0001f}, {0.0001f, 0, 1}};
void intoFrame(Mesh& m, const RestFrame& f)
{
	for (size_t i = 0; i < m.corners.size(); i++) {
		V rel = sub(v(m.corners[i].x, m.corners[i].y, m.corners[i].z), f.at);
		m.corners[i].x = dot(rel, f.x), m.corners[i].y = dot(rel, f.y), m.corners[i].z = dot(rel, f.z);
	}
}
const Rgb kSanta = {196, 22, 32}, kFur = {248, 248, 250}, kBelt = {26, 24, 28}, kGold = {232, 184, 60},
          kHolly = {30, 120, 52};
// a closed ring of points (an ellipse round y): side half-width a, front z0 + b, back z0 - b2, at height y,
// the front `lift` higher than the back
void ringTube(Mesh& m, float y, float a, float z0, float b, float b2, float lift, float r, Rgb col, int seg = 32)
{
	std::vector<V> c;
	std::vector<float> rr;
	for (int i = 0; i <= seg; i++) {
		float t = 2 * kPi * i / seg, co = cosf(t);
		c.push_back(v(a * sinf(t), y + lift * co, z0 + (co > 0 ? b : b2) * co));
		rr.push_back(r);
	}
	m.tube(c, rr, col, 10, false);
}
// Santa's hat over Mario's cap: a fur brim round it (the cap's peak still shows under it in front), a soft
// red cone flopping back, a fur pom-pom at its end
void santaHatMario(Mesh& m)
{
	m.light = norm(v(0.2f, 0.9f, 0.4f));
	ringTube(m, 119, 30.5f, -5.5f, 30, 30, 8, 5.5f, kFur, 40);
	std::vector<V> c;
	std::vector<float> r;
	const int n = 14;
	for (int i = 0; i <= n; i++) {
		float t = (float)i / n;
		c.push_back(v(6 * t * t, 121 + 46 * t - 20 * t * t, -5 - 8 * t - 36 * t * t));
		r.push_back(31 * (1 - t) * (1 - 0.25f * t) + 4 * t);
	}
	m.tube(c, r, kSanta, 22);
	m.sphere(c.back(), 8.5f, kFur, 14, 9);
	intoFrame(m, kMarioHead);
}
// a black belt with a gold buckle round his waist
void santaBelt(Mesh& m)
{
	m.light = norm(v(0.2f, 0.9f, 0.4f));
	const int seg = 36;
	std::vector<V> lo, hi;
	for (int i = 0; i <= seg; i++) {
		float t = 2 * kPi * i / seg, co = cosf(t);
		float a = 30.0f, z = 2 + (co > 0 ? 24 : 26.5f) * co; // a little outside the shirt all round
		lo.push_back(v(a * sinf(t), 54.5f, z));
		hi.push_back(v(a * sinf(t) * 1.01f, 61.5f, 2 + (z - 2) * 1.01f));
	}
	for (int i = 0; i < seg; i++)
		m.quad(lo[i], lo[i + 1], hi[i + 1], hi[i], kBelt);
	m.cylinder(v(0, 58, 25.2f), v(0, 58, 27.6f), 6, 6, kGold, 4);  // the buckle, square
	m.cylinder(v(0, 58, 27.5f), v(0, 58, 27.9f), 3.2f, 3.2f, kBelt, 4);
	intoFrame(m, kMarioWaist);
}
// white fur round each sleeve's end (the glove starts at 40 out) and each trouser leg's, over the shoe
void santaCuffR(Mesh& m)
{
	m.light = norm(v(0.2f, 0.9f, 0.4f));
	std::vector<V> c;
	std::vector<float> r;
	for (int i = 0; i <= 28; i++) {
		float t = 2 * kPi * i / 28;
		c.push_back(v(-37.5f, 80 + 9.6f * sinf(t), -3.3f + 9.6f * cosf(t)));
		r.push_back(3.4f);
	}
	m.tube(c, r, kFur, 10, false);
	intoFrame(m, kMarioArmR);
}
void santaCuffL(Mesh& m)
{
	m.light = norm(v(0.2f, 0.9f, 0.4f));
	std::vector<V> c;
	std::vector<float> r;
	for (int i = 0; i <= 28; i++) {
		float t = 2 * kPi * i / 28;
		c.push_back(v(37.5f, 80 + 9.6f * sinf(t), -3.3f + 9.6f * cosf(t)));
		r.push_back(3.4f);
	}
	m.tube(c, r, kFur, 10, false);
	intoFrame(m, kMarioArmL);
}
void santaLegCuff(Mesh& m, float side, const RestFrame& f)
{
	m.light = norm(v(0.2f, 0.9f, 0.4f));
	std::vector<V> c;
	std::vector<float> r;
	for (int i = 0; i <= 28; i++) {
		float t = 2 * kPi * i / 28;
		c.push_back(v(side * 11.2f + 12.2f * sinf(t), 20.5f, 0.6f + 12.6f * cosf(t)));
		r.push_back(3.8f);
	}
	m.tube(c, r, kFur, 10, false);
	intoFrame(m, f);
}
void santaLegCuffR(Mesh& m) { santaLegCuff(m, -1, kMarioLegR); }
void santaLegCuffL(Mesh& m) { santaLegCuff(m, 1, kMarioLegL); }
// Santa's beard (the "santa_beard" option, Off): fluffy white round his chin and jaw, a white moustache over
// his own
void santaBeard(Mesh& m)
{
	m.light = norm(v(0.2f, 0.9f, 0.4f));
	// rows of soft lumps round his jaw and chin, fuller and longer at the front
	for (int row = 0; row < 4; row++) {
		float y = 92 - row * 5.5f, reach = 1.0f - row * 0.12f;
		for (int i = -6; i <= 6; i++) {
			float a = i * 0.24f * reach;
			m.sphere(v(18.5f * sinf(a) * reach, y - 2.5f * cosf(a) - row * 2.0f * cosf(a), 8 + 16 * cosf(a)), 7 - row * 0.5f,
			         kFur, 10, 7);
		}
	}
	m.sphere(v(0, 74, 18), 9, kFur, 12, 8); // its point
	intoFrame(m, kMarioHead);
}
// a white moustache over his own, with the beard
void santaMoustache(Mesh& m)
{
	m.light = norm(v(0.2f, 0.9f, 0.4f));
	m.sphere(v(-8, 96.5f, 31), 6.2f, kFur, 10, 7);
	m.sphere(v(8, 96.5f, 31), 6.2f, kFur, 10, 7);
	m.sphere(v(-15, 95, 27), 5, kFur, 10, 7);
	m.sphere(v(15, 95, 27), 5, kFur, 10, 7);
	intoFrame(m, kMarioHead);
}
// His own moustache gone (the "santa_no_moustache" option, with the beard): its triangles in his face
// (ma_mdl1's _mat_head(8), those with a dark corner under his nose), a hair out from it, in his skin's colour
const float kMoustache[][3][3] = {
	{{2.25f, 93.12f, 22.08f}, {5.26f, 93.82f, 21.62f}, {4.64f, 102.48f, 22.95f}},
	{{4.66f, 102.49f, 22.94f}, {5.28f, 93.83f, 21.62f}, {7.54f, 95.42f, 21.26f}},
	{{4.68f, 102.50f, 22.94f}, {7.56f, 95.43f, 21.25f}, {7.90f, 103.04f, 21.98f}},
	{{7.87f, 103.04f, 21.99f}, {7.53f, 95.43f, 21.26f}, {9.96f, 95.51f, 20.77f}},
	{{7.92f, 103.06f, 21.97f}, {10.02f, 95.53f, 20.76f}, {10.59f, 103.89f, 21.00f}},
	{{10.65f, 103.89f, 20.98f}, {10.07f, 95.53f, 20.73f}, {12.63f, 96.44f, 19.28f}},
	{{10.62f, 103.88f, 20.99f}, {12.60f, 96.43f, 19.29f}, {14.48f, 100.11f, 18.71f}},
	{{0.03f, 93.81f, 22.46f}, {0.03f, 97.77f, 25.65f}, {-2.16f, 92.95f, 22.01f}},
	{{-0.09f, 97.81f, 25.66f}, {-5.28f, 93.69f, 21.57f}, {-2.28f, 92.99f, 22.03f}},
	{{-0.06f, 97.79f, 25.65f}, {-6.03f, 97.75f, 24.35f}, {-5.26f, 93.67f, 21.56f}},
	{{-6.14f, 97.76f, 24.30f}, {-7.64f, 95.27f, 21.15f}, {-5.37f, 93.68f, 21.52f}},
	{{-6.03f, 97.69f, 24.30f}, {-9.95f, 95.29f, 20.66f}, {-7.52f, 95.21f, 21.15f}},
	{{-0.05f, 93.97f, 22.53f}, {-0.05f, 102.29f, 24.05f}, {-2.24f, 93.11f, 22.08f}},
	{{-2.27f, 93.12f, 22.07f}, {-0.09f, 102.30f, 24.04f}, {-4.66f, 102.48f, 22.94f}},
	{{-2.25f, 93.12f, 22.08f}, {-4.64f, 102.48f, 22.95f}, {-5.26f, 93.82f, 21.62f}},
	{{-5.28f, 93.83f, 21.62f}, {-4.67f, 102.49f, 22.94f}, {-7.55f, 95.42f, 21.25f}},
	{{-10.09f, 95.45f, 20.70f}, {-6.16f, 97.85f, 24.34f}, {-12.64f, 96.36f, 19.25f}},
	{{-6.16f, 97.84f, 24.34f}, {-13.84f, 101.53f, 20.55f}, {-12.64f, 96.35f, 19.25f}},
	{{-6.07f, 98.06f, 24.38f}, {-7.90f, 103.19f, 21.96f}, {-13.76f, 101.75f, 20.59f}},
	{{-7.90f, 103.16f, 21.97f}, {-10.57f, 103.98f, 21.00f}, {-13.75f, 101.72f, 20.61f}},
	{{14.60f, 100.03f, 18.58f}, {13.93f, 101.53f, 20.47f}, {12.72f, 96.35f, 19.16f}},
	{{13.84f, 101.53f, 20.55f}, {6.15f, 97.84f, 24.34f}, {12.64f, 96.35f, 19.25f}},
	{{13.75f, 101.75f, 20.60f}, {7.90f, 103.19f, 21.96f}, {6.07f, 98.06f, 24.38f}},
	{{7.87f, 103.20f, 21.96f}, {4.65f, 102.66f, 22.92f}, {6.04f, 98.07f, 24.39f}},
	{{10.64f, 103.91f, 20.98f}, {14.50f, 100.14f, 18.70f}, {12.81f, 104.87f, 19.70f}},
	{{12.79f, 104.86f, 19.70f}, {14.49f, 100.14f, 18.70f}, {16.90f, 100.88f, 17.45f}},
	{{12.84f, 104.93f, 19.67f}, {16.95f, 100.95f, 17.41f}, {17.72f, 103.53f, 16.22f}},
	{{-12.73f, 96.35f, 19.16f}, {-13.93f, 101.53f, 20.47f}, {-14.60f, 100.03f, 18.58f}},
	{{-13.83f, 101.44f, 20.50f}, {-16.92f, 100.68f, 17.36f}, {-14.51f, 99.94f, 18.62f}},
	{{-13.91f, 101.68f, 20.52f}, {-17.77f, 103.50f, 16.19f}, {-17.00f, 100.92f, 17.37f}},
	{{-15.83f, 106.63f, 16.97f}, {-17.72f, 103.53f, 16.22f}, {-12.85f, 104.93f, 19.66f}},
	{{-12.85f, 104.93f, 19.66f}, {-17.72f, 103.53f, 16.22f}, {-16.95f, 100.95f, 17.41f}},
	{{-12.80f, 104.86f, 19.70f}, {-16.91f, 100.88f, 17.44f}, {-14.49f, 100.14f, 18.70f}},
	{{-12.81f, 104.87f, 19.69f}, {-14.51f, 100.14f, 18.69f}, {-10.65f, 103.91f, 20.97f}},
	{{-10.63f, 103.88f, 20.99f}, {-14.49f, 100.11f, 18.70f}, {-12.61f, 96.43f, 19.29f}},
	{{-10.65f, 103.89f, 20.97f}, {-12.63f, 96.44f, 19.28f}, {-10.07f, 95.53f, 20.73f}},
	{{-10.60f, 103.89f, 21.00f}, {-10.02f, 95.53f, 20.76f}, {-7.93f, 103.06f, 21.97f}},
	{{-7.87f, 103.04f, 21.98f}, {-9.97f, 95.51f, 20.77f}, {-7.53f, 95.43f, 21.26f}},
	{{-7.91f, 103.04f, 21.97f}, {-7.57f, 95.43f, 21.25f}, {-4.69f, 102.50f, 22.94f}},
	{{7.89f, 103.16f, 21.97f}, {13.75f, 101.72f, 20.61f}, {10.56f, 103.98f, 21.00f}},
	{{13.78f, 101.76f, 20.58f}, {12.76f, 104.99f, 19.70f}, {10.59f, 104.03f, 20.98f}},
	{{13.82f, 101.77f, 20.56f}, {15.79f, 106.70f, 16.98f}, {12.80f, 105.00f, 19.67f}},
	{{12.64f, 96.36f, 19.25f}, {6.16f, 97.85f, 24.34f}, {10.08f, 95.45f, 20.71f}},
	{{6.02f, 97.69f, 24.30f}, {7.51f, 95.21f, 21.15f}, {9.95f, 95.29f, 20.66f}},
	{{6.14f, 97.76f, 24.30f}, {5.37f, 93.68f, 21.52f}, {7.63f, 95.27f, 21.15f}},
	{{15.85f, 106.65f, 16.95f}, {13.89f, 101.72f, 20.53f}, {17.75f, 103.55f, 16.19f}},
	{{13.90f, 101.68f, 20.52f}, {16.99f, 100.92f, 17.38f}, {17.76f, 103.50f, 16.19f}},
	{{13.83f, 101.44f, 20.51f}, {14.50f, 99.94f, 18.62f}, {16.92f, 100.68f, 17.36f}},
	{{-0.07f, 98.11f, 25.70f}, {-0.07f, 102.47f, 24.03f}, {-6.04f, 98.07f, 24.39f}},
	{{-0.07f, 102.48f, 24.03f}, {-4.65f, 102.66f, 22.92f}, {-6.04f, 98.07f, 24.39f}},
	{{-10.60f, 104.03f, 20.98f}, {-12.76f, 104.99f, 19.70f}, {-13.78f, 101.76f, 20.58f}},
	{{-12.81f, 105.00f, 19.67f}, {-15.80f, 106.70f, 16.97f}, {-13.83f, 101.77f, 20.55f}},
	{{0.04f, 93.97f, 22.53f}, {2.23f, 93.11f, 22.08f}, {0.04f, 102.29f, 24.05f}},
	{{0.08f, 102.30f, 24.04f}, {2.27f, 93.12f, 22.08f}, {4.66f, 102.48f, 22.94f}},
	{{4.65f, 102.66f, 22.92f}, {0.07f, 102.48f, 24.03f}, {6.04f, 98.07f, 24.39f}},
	{{0.07f, 102.47f, 24.03f}, {0.07f, 98.11f, 25.70f}, {6.04f, 98.07f, 24.39f}},
	{{-0.03f, 93.81f, 22.46f}, {2.16f, 92.95f, 22.01f}, {-0.03f, 97.77f, 25.65f}},
	{{2.27f, 92.99f, 22.03f}, {5.28f, 93.69f, 21.57f}, {0.08f, 97.81f, 25.66f}},
	{{-0.16f, 97.83f, 21.30f}, {-0.16f, 97.31f, 24.26f}, {5.05f, 99.97f, 23.96f}},
	{{0.15f, 97.31f, 24.26f}, {0.15f, 97.83f, 21.30f}, {-5.06f, 99.97f, 23.96f}},
	{{-6.05f, 98.07f, 24.38f}, {-4.65f, 102.66f, 22.92f}, {-7.88f, 103.20f, 21.96f}},
	{{0.19f, 97.73f, 21.45f}, {-6.89f, 104.56f, 20.96f}, {-5.02f, 99.87f, 24.11f}},
	{{6.89f, 104.56f, 20.96f}, {-0.19f, 97.73f, 21.45f}, {5.02f, 99.87f, 24.11f}},
	{{-17.75f, 103.55f, 16.19f}, {-13.89f, 101.72f, 20.52f}, {-15.86f, 106.65f, 16.94f}},
	{{6.03f, 97.75f, 24.35f}, {0.06f, 97.79f, 25.65f}, {5.25f, 93.67f, 21.56f}},
	{{15.83f, 106.63f, 16.98f}, {12.84f, 104.93f, 19.67f}, {17.72f, 103.53f, 16.22f}},
};
void moustacheCover(Mesh& m)
{
	m.light = norm(v(0.2f, 0.6f, 0.8f));
	const Rgb skin = {255, 186, 146};
	for (size_t i = 0; i < sizeof kMoustache / sizeof kMoustache[0]; i++)
		m.tri(v(kMoustache[i][0][0], kMoustache[i][0][1], kMoustache[i][0][2]),
		      v(kMoustache[i][1][0], kMoustache[i][1][1], kMoustache[i][1][2]),
		      v(kMoustache[i][2][0], kMoustache[i][2][1], kMoustache[i][2][2]), skin);
	intoFrame(m, kMarioHead);
}
// FLUDD wrapped as a present: a gold ribbon round its tank and along its back, a bow where they cross, and a
// sprig of holly beside it. The tank is a drum along FLUDD's x (from -3 to 33), round y -40, z 0, 19 across;
// its back faces -y (away from Mario)
void fluddRibbon(Mesh& m)
{
	m.light = norm(v(0.3f, -0.8f, 0.4f));
	std::vector<V> c;
	std::vector<float> r;
	for (int i = 0; i <= 32; i++) { // round the tank
		float t = 2 * kPi * i / 32;
		c.push_back(v(15, -40 + 20.3f * cosf(t), 20.3f * sinf(t)));
		r.push_back(1.9f);
	}
	m.tube(c, r, kGold, 8, false);
	{ // along its back, following the tank's shape (its radius along x, measured on the model)
		const float px[] = {-2.5f, 3, 8, 12, 15, 18, 22, 25, 27, 29.5f, 32.5f};
		const float pr[] = {10.5f, 16.4f, 18.2f, 18.9f, 18.9f, 18.6f, 17.3f, 15, 13.3f, 15.8f, 15.4f};
		std::vector<V> line;
		std::vector<float> lr;
		for (int i = 0; i < 11; i++) {
			line.push_back(v(px[i], -40 - pr[i] - 1.3f, 0));
			lr.push_back(1.9f);
		}
		m.tube(line, lr, kGold, 8);
	}
	// the bow: two loops and two tails, from the crossing
	V knot = v(15, -61.2f, 0);
	for (int s = -1; s <= 1; s += 2) {
		std::vector<V> loop;
		std::vector<float> lr;
		for (int i = 0; i <= 12; i++) {
			float t = 2 * kPi * i / 12;
			loop.push_back(add(knot, v(4.5f * sinf(t), -3.5f - 3.5f * (1 - cosf(t)) * 0.5f, s * (6 + 6 * (1 - cosf(t)) * 0.5f))));
			lr.push_back(2.3f);
		}
		m.tube(loop, lr, kSanta, 8, false);
		m.cylinder(knot, add(knot, v(-11, -1.5f, s * 5)), 2.2f, 1.4f, kSanta, 6);
	}
	m.sphere(knot, 3.4f, kSanta, 10, 7);
	// holly: two leaves and three berries, beside the bow
	V h = v(23, -58.5f, 5);
	for (int s = -1; s <= 1; s += 2)
		m.cylinder(h, add(h, v(5, -1.5f, s * 6)), 2.6f, 0.4f, kHolly, 6);
	for (int k = 0; k < 3; k++)
		m.sphere(add(h, v(-1 + 1.8f * k, -1.8f, (k - 1) * 1.6f)), 1.5f, Rgb{214, 20, 30}, 8, 6);
	intoFrame(m, kFluddMain);
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
	SmsMesh antlers = 0, nose = 0, noseSnow = 0;
	SmsMesh sHat = 0, sBelt = 0, sCuffR = 0, sCuffL = 0, sLegR = 0, sLegL = 0, sBeard = 0, sTache = 0, sNoTache = 0, fluddDeco = 0;
	bool capOff = false; // his own cap left out
	bool santaOn = false, fluddOn = false; // tints put on
	bool rudolphOn = false; // Yoshi's colour set
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
		float hm[3][4];
		if (!y || !parts->joint(y, "jnt_head", hm))
			return;
		draw(antlers, hm);
		draw(nose, hm);
		if (config().get_int(handle(), "rudolph_nose_snow", 0))
			draw(noseSnow, hm);
		// the nose's glow, facing the camera, pulsing a little
		if (cam && puffTex) {
			V l = restToHead(kNoseAt), at;
			at.x = hm[0][0] * l.x + hm[0][1] * l.y + hm[0][2] * l.z + hm[0][3];
			at.y = hm[1][0] * l.x + hm[1][1] * l.y + hm[1][2] * l.z + hm[1][3];
			at.z = hm[2][0] * l.x + hm[2][1] * l.y + hm[2][2] * l.z + hm[2][3];
			float scale = sqrtf(hm[0][0] * hm[0][0] + hm[1][0] * hm[1][0] + hm[2][0] * hm[2][0]);
			V right = norm(v(cam->view[0][0], cam->view[0][1], cam->view[0][2]));
			V camUp = norm(v(cam->view[1][0], cam->view[1][1], cam->view[1][2]));
			float g = (34 + 4 * sinf(now * 3.0f)) * scale;
			V hr = mul(right, g), hu = mul(camUp, g);
			std::vector<SmsTexCorner> c;
			quad(c, add(sub(at, hr), hu), add(add(at, hr), hu), sub(add(at, hr), hu), sub(sub(at, hr), hu), 255, 40, 30, 140);
			api().models->draw_textured(&c[0], (int)c.size(), puffTex, SMS_BLEND_ADD);
		}
	}

	// a first-person mod's camera is on: Mario's head is left out, so his hat and beard are too
	bool firstPerson()
	{
		if (!SMS_API_HAS(&api(), held_fludd) || api().held_fludd->magic != SMS_HELD_FLUDD_MAGIC
		    || !SMS_API_HAS(api().held_fludd, get_state))
			return false;
		SmsHeldFluddState st;
		memset(&st, 0, sizeof st);
		st.size = sizeof st;
		return api().held_fludd->get_state(&st) && st.first_person;
	}
	void drawOn(SmsObject who, const char* joint, SmsMesh mesh)
	{
		float m[3][4];
		if (mesh && parts->joint(who, joint, m))
			draw(mesh, m);
	}

	// Santa Mario: his clothes Santa red, his shoes black boots, a Santa hat, a belt, fur cuffs (and a beard)
	// Christmas FLUDD: red and green, with gold handles, wrapped in a gold ribbon with a bow and holly
	void santa()
	{
		if (!partsHave(offsetof(SmsPartsApi, yoshi_color) + sizeof(void*)))
			return;
		const bool suit = config().get_int(handle(), "santa_mario", 1) != 0;
		const bool xf   = config().get_int(handle(), "christmas_fludd", 1) != 0;
		SmsObject mario = parts->player_part(SMS_PARTS_MARIO);
		SmsObject fludd = parts->player_part(SMS_PARTS_FLUDD);
		if (mario && (suit || santaOn)) {
			// the body's materials are _mat_head, _mat_head(2)..(8) (one picture for all): (2) the overalls'
			// bib, _mat_head the shirt and trousers, (3) and (4) the shoes, (5) and (6) the gloves, (7) the
			// cap, (8) the face. A tint holds for every name holding the one given, so all first, then back.
			// his own cap off under the Santa hat: the cap model (and the hair shown without it) and the
			// cap in his body's model (_mat_head(7))
			if (suit != capOff && partsHave(offsetof(SmsPartsApi, hide) + sizeof(void*))
			    && SMS_API_HAS(&api(), player) && player().set_hidden_parts) {
				player().set_hidden_parts(suit ? SMS_PLAYER_PART_CAP : 0);
				parts->hide(mario, "_mat_head(7)", suit ? 1 : 0);
				capOff = suit;
			}
			if (suit) {
				parts->tint(mario, "_mat_head", 0xB8121CF0u);
				parts->tint(mario, "_mat_head(", 0);
				parts->tint(mario, "_mat_head(2)", 0xB8121CF0u);
				parts->tint(mario, "_mat_head(3)", 0x1C1A1EE6u);
				parts->tint(mario, "_mat_head(4)", 0x1C1A1EE6u);
			} else
				parts->tint(mario, "", 0);
			santaOn = suit;
		}
		if (fludd && (xf || fluddOn)) {
			if (xf) {
				parts->tint(fludd, "_mat_G_main", 0xC0182AE8u);
				parts->tint(fludd, "_mat_G_main_tank", 0);
				parts->tint(fludd, "_mat_froater(2)", 0x1C8A3CE8u);
				parts->tint(fludd, "_mat_froater(3)", 0xE0B03CA0u);
			} else
				parts->tint(fludd, "", 0);
			fluddOn = xf;
		}
		if (mario && suit) {
			if (!firstPerson()) {
				drawOn(mario, "jnt_head", sHat);
				if (config().get_int(handle(), "santa_beard", 0)) {
					drawOn(mario, "jnt_head", sBeard);
					if (config().get_int(handle(), "santa_no_moustache", 0))
						drawOn(mario, "jnt_head", sNoTache);
					else
						drawOn(mario, "jnt_head", sTache);
				}
			}
			drawOn(mario, "jnt_waist", sBelt);
			drawOn(mario, "jnt_arm_R2", sCuffR);
			drawOn(mario, "jnt_arm_L2", sCuffL);
			drawOn(mario, "jnt_leg_R2", sLegR);
			drawOn(mario, "jnt_leg_L2", sLegL);
		}
		if (fludd && xf)
			drawOn(fludd, "jnt_G_main", fluddDeco);
	}

	void tick()
	{
		{
			SmsCamera cam;
			memset(&cam, 0, sizeof cam);
			cam.size = sizeof cam;
			bool camOk = SMS_API_HAS(&api(), camera) && api().camera->get(&cam) && cam.valid;
			rudolph(camOk ? &cam : nullptr);
			santa();
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
		optionBool("rudolph_nose_snow", "A little snow on Rudolph's nose", false);
		optionBool("santa_mario", "Mario dressed as Santa", true);
		optionBool("santa_beard", "A white beard on Santa Mario", false);
		optionBool("santa_no_moustache", "No moustache with Santa's beard", false);
		optionBool("christmas_fludd", "FLUDD wrapped for Christmas", true);

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
		noseSnow = make(rudolphNoseSnow);
		sHat     = make(santaHatMario);
		sBelt    = make(santaBelt);
		sCuffR   = make(santaCuffR);
		sCuffL   = make(santaCuffL);
		sLegR    = make(santaLegCuffR);
		sLegL    = make(santaLegCuffL);
		sBeard   = make(santaBeard);
		sTache   = make(santaMoustache);
		sNoTache = make(moustacheCover);
		fluddDeco = make(fluddRibbon);
		hat     = make(santaHat);
		head    = make(snowmanHead);
		buttons = make(snowmanButtons);
		arms    = make(snowmanArms);

		on(SMS_EV_STAGE_LOADED, [this](SmsEvent&) { dressed.clear(), snowy.clear(); }); // the tints went with the old stage
		on(SMS_EV_TICK, [this](SmsEvent&) { tick(); });
		info("loaded: Santa hats, winter clothes, snowmen, Rudolph, Santa Mario and Christmas FLUDD");
		return 0;
	}
};

} // namespace

SMS_MOD_CLASS(ChristmasAdditions)
