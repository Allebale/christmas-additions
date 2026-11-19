// Christmas Additions: Christmas in Isle Delfino, drawn by the mod, with no game code of its own.
//
// - Santa hats: every Pianta wears one, drawn each tick at its head joint (api->parts' "kubi").
// - Winter clothes: each Pianta's clothes ("fuku" materials) drawn in a winter colour.
// - Snowmen: every Strollin' Stu stack (Dango: TDangoHamuKuri, TBossDangoHamuKuri) is drawn white,
//   and its top ball wears a top hat, a scarf, coal eyes and mouth and a carrot nose; the balls
//   under it have coal buttons, the one under the head twig arms. A ball knocked off the top
//   leaves the next one down as the head.
//
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

// ---------------------------------------------------------------- the mod

struct ChristmasAdditions : sms::Mod {
	using sms::Mod::Mod;

	const SmsPartsApi* parts = nullptr;
	SmsMesh hat = 0, head = 0, buttons = 0, arms = 0;
	std::set<SmsObject> dressed, snowy; // Piantas' clothes and snowmen tinted, this stage
	int leftArea = -1, leftEpisode = -1; // the stage Mario left by its way out, till he starts in the next

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

	void tick()
	{
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
		hat     = make(santaHat);
		head    = make(snowmanHead);
		buttons = make(snowmanButtons);
		arms    = make(snowmanArms);

		on(SMS_EV_STAGE_LOADED, [this](SmsEvent&) { dressed.clear(), snowy.clear(); }); // the tints went with the old stage
		on(SMS_EV_TICK, [this](SmsEvent&) { tick(); });
		info("loaded: Santa hats, winter clothes and snowmen");
		return 0;
	}
};

} // namespace

SMS_MOD_CLASS(ChristmasAdditions)
