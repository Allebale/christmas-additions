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

#include <map>
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
	static V sideOf(V axis)
	{
		V a = norm(axis);
		return norm(cross(a, fabsf(a.x) < 0.9f ? v(1, 0, 0) : v(0, 1, 0)));
	}
	static void ring(V c, V axis, float r, int seg, std::vector<V>& out) { ring(c, axis, sideOf(axis), r, seg, out); }
	// `u`: a direction across the axis where the ring starts
	static void ring(V c, V axis, V u, float r, int seg, std::vector<V>& out)
	{
		V a = norm(axis);
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
		// each ring turned on from the one before (not picked afresh), so a bending tube never twists
		std::vector<std::vector<V> > rings(c.size());
		V u = v(1, 0, 0);
		const size_t n = c.size();
		V gap = sub(c[n - 1], c[0]);
		const bool closed = n > 3 && dot(gap, gap) < 1e-4f; // a loop: its last ring is its first, no seam
		for (size_t i = 0; i < n; i++) {
			if (closed && i == n - 1) {
				rings[i] = rings[0];
				break;
			}
			V axis = i + 1 < n ? (i > 0 ? sub(c[i + 1], c[i - 1]) : (closed ? sub(c[1], c[n - 2]) : sub(c[1], c[0])))
			                   : sub(c[i], c[i - 1]);
			V a = norm(axis);
			u = i == 0 ? sideOf(a) : norm(sub(u, mul(a, dot(u, a))));
			ring(c[i], a, u, r[i], seg, rings[i]);
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
	// snow on the hat: a soft cap on its top, a little lip over its edge, and a ring on the brim
	m.cylinder(v(x0 + 59, 0, 0), v(x0 + 65, 0, 0), 35, 31, kWhite, 20);
	m.cylinder(v(x0 + 65, 0, 0), v(x0 + 69, 0, 0), 31, 22, kWhite, 20);
	m.torus(x0 + 60, 33, 3, kWhite, 24, 6);
	m.torus(x0 + 6, 42, 3.5f, kWhite, 24, 6);
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

// Bob-ombs dressed as Christmas baubles (the level names them "Bauble ..."): in a frame round the body's middle,
// x up, the body 1 round. A cap and its loop on top, and one of five patterns, each in its own colour.
const Rgb kSilver = {214, 220, 230}, kBaubleGold = {236, 190, 60};
void baubleCap(Mesh& m)
{
	m.cylinder(v(0.9f, 0, 0), v(1.16f, 0, 0), 0.30f, 0.27f, kBaubleGold, 14);
	m.torus(0.94f, 0.31f, 0.035f, kBaubleGold, 16, 6);
	std::vector<V> loop;
	std::vector<float> r;
	for (int i = 0; i <= 16; i++) {
		float t = kPi * 2 * i / 16;
		loop.push_back(v(1.28f + 0.12f * cosf(t + kPi), 0.12f * sinf(t), 0));
		r.push_back(0.032f);
	}
	m.tube(loop, r, kBaubleGold, 8, false);
}
void baublePattern(Mesh& m, int kind, Rgb a, Rgb b)
{
	switch (kind) {
	case 0: // one wide band round the middle
		m.torus(0, 1.0f, 0.09f, a, 32, 8);
		break;
	case 1: // two thin bands
		for (int s = -1; s <= 1; s += 2)
			m.torus(0.38f * s, sqrtf(1 - 0.38f * 0.38f), 0.05f, a, 32, 6);
		break;
	case 2: { // dots all over
		unsigned h = 7u;
		for (int i = 0; i < 22; i++) {
			h = h * 1103515245u + 12345u;
			float up = -60 + 120.0f * ((h >> 8) & 1023) / 1023.0f;
			h = h * 1103515245u + 12345u;
			float side = 360.0f * ((h >> 8) & 1023) / 1023.0f;
			m.sphere(onBall(v(0, 0, 0), 1.0f, up, side), 0.07f, i % 2 ? a : b, 6, 4);
		}
		break;
	}
	case 3: // a zigzag of little beads round the middle
		for (int i = 0; i < 24; i++)
			m.sphere(onBall(v(0, 0, 0), 1.01f, (i % 2 ? 12.0f : -12.0f), 15.0f * i), 0.06f, a, 6, 4);
		m.torus(0.55f, sqrtf(1 - 0.55f * 0.55f), 0.035f, b, 28, 5);
		m.torus(-0.55f, sqrtf(1 - 0.55f * 0.55f), 0.035f, b, 28, 5);
		break;
	default: // stripes from top to bottom
		for (int k = 0; k < 8; k++) {
			std::vector<V> c;
			std::vector<float> r;
			for (int i = 0; i <= 12; i++) {
				c.push_back(onBall(v(0, 0, 0), 1.005f, -80.0f + 160.0f * i / 12, 45.0f * k));
				r.push_back(0.045f);
			}
			m.tube(c, r, k % 2 ? a : b, 6, false);
		}
	}
}

// Gingerbread men (the mod's own, walking about where a level marks with invisible coins named "Gingerbread N"):
// a cookie figure in pieces that swing as he walks, tall and long-legged as Il Piantissimo. Built round
// (tubes and balls) in his own frame, x to his right, y up, z forward, and drawn squashed to a cookie's
// thickness. Each piece hangs from its joint: the body and head, two arms (from the shoulder), two legs (hip).
const Rgb kCookie = {178, 104, 46}, kIcing = {250, 250, 252}, kGumRed = {214, 36, 48}, kGumGreen = {44, 168, 70}, kChoc = {52, 30, 20};
const float kHipY = 86, kShoulderY = 146, kHipX = 19, kShoulderX = 33, kHeadR = 37;
const Rgb kCheek = {236, 120, 120};
// a wavy line of icing across a limb's front, round `at`, `w` wide
static void icingWave(Mesh& m, V at, float w, float frontZ)
{
	std::vector<V> c;
	std::vector<float> r;
	for (int i = 0; i <= 10; i++) {
		float t = i / 10.0f;
		c.push_back(v(at.x - w / 2 + w * t, at.y + 2.5f * sinf(t * kPi * 3), frontZ));
		r.push_back(1.9f);
	}
	m.tube(c, r, kIcing, 6, false);
}
void gingerBody(Mesh& m)
{
	const float headY = kShoulderY + kHeadR + 2;
	m.cylinder(v(0, kHipY - 8, 0), v(0, kShoulderY, 0), 37, 34, kCookie, 18);   // a wide, round body
	m.sphere(v(0, kHipY - 8, 0), 37, kCookie, 16, 8);
	m.sphere(v(0, kShoulderY, 0), 34, kCookie, 16, 8);
	m.sphere(v(0, headY, 0), kHeadR, kCookie, 18, 12);                          // a big round head
	for (int s = -1; s <= 1; s += 2) {
		m.sphere(v(12.0f * s, headY + 7, 34), 6.0f, kChoc, 10, 6);              // round chocolate eyes
		m.sphere(v(12.0f * s + 2, headY + 9.5f, 39), 1.8f, kIcing, 6, 4);        // a glint in each
		m.sphere(v(21.0f * s, headY - 6, 30), 5.5f, kCheek, 10, 6);             // rosy cheeks
	}
	std::vector<V> c;                                                            // a smile: one icing line
	std::vector<float> r;
	for (int i = 0; i <= 12; i++) {
		float t = (i - 6) / 6.0f, x = 13 * t, y = headY - 8 - 6 * (1 - t * t);
		float z = sqrtf(kHeadR * kHeadR - x * x - (y - headY) * (y - headY)) + 0.5f;
		c.push_back(v(x, y, z));
		r.push_back(2.0f);
	}
	m.tube(c, r, kIcing, 6, false);
	for (int i = 0; i < 3; i++)                                                  // gumdrop buttons
		m.sphere(v(0, kHipY + 8 + 21.0f * i, 36), 7, i == 1 ? kGumGreen : kGumRed, 10, 6);
}
// an arm from its shoulder (at the origin), down and out: side -1 left, +1 right
void gingerArm(Mesh& m, int side)
{
	V hand = v(30.0f * side, -46, 0);
	m.cylinder(v(0, 0, 0), hand, 16, 15, kCookie, 12);
	m.sphere(v(0, 0, 0), 16, kCookie, 10, 6);
	m.sphere(hand, 16, kCookie, 12, 8);
	icingWave(m, add(mul(hand, 0.72f), v(0, 0, 0)), 24, 15.5f);
}
// a leg from the hip (at the origin) to a round foot
void gingerLeg(Mesh& m, int side)
{
	V foot = v(9.0f * side, -kHipY + 18, 0);
	m.cylinder(v(0, 0, 0), foot, 18, 17, kCookie, 12);
	m.sphere(foot, 18, kCookie, 12, 8);
	icingWave(m, mul(foot, 0.8f), 26, 17.5f);
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
// His face, cap and moustache hang on "M_head" (a child of jnt_head, at rest in the same place), which the game
// turns on its own as he looks about: what goes on his face goes there too
const char kFaceJoint[] = "M_head";
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
void costumeHat(Mesh& m, Rgb body, bool bell)
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
	m.tube(c, r, body, 22);
	if (bell) { // a little gold bell at its end, its mouth down
		V tip = c.back();
		m.cylinder(add(tip, v(0, 1, 0)), add(tip, v(0, -7, 0)), 3, 6.5f, kGold, 12);
		m.sphere(add(tip, v(0, -7.5f, 0)), 1.6f, Rgb{150, 110, 30}, 8, 6);
	} else
		m.sphere(c.back(), 8.5f, kFur, 14, 9);
	intoFrame(m, kMarioHead);
}
void santaHatMario(Mesh& m) { costumeHat(m, kSanta, false); }
// The elf's: the same hat in green, a bell at its end
const Rgb kElf = {30, 130, 56};
void elfHatMario(Mesh& m) { costumeHat(m, kElf, true); }
// Pointed elf ears over his own (his ears stand out from 22 to 31 at 98 to 117 high, round x 28, z -7): each a
// flat leaf of skin, rounded at its foot and long to a point up, out and back past the hat's brim, cupped on
// its front with a rim round its edge, thin, and the hollow a little darker
void elfEar(Mesh& m, float side)
{
	const V base = v(side * 24.5f, 106, -5.5f);
	const V L = norm(v(side * 0.62f, 0.7f, -0.36f));   // along it, to its point
	V N = norm(v(side * 0.45f, 0.05f, 1));              // the way its front faces: forward and out
	N = norm(sub(N, mul(L, dot(N, L))));
	const V W = cross(N, L);                            // across it
	const float len = 25, wide = 7.5f, thick = 1.6f, cup = 2.2f;
	const Rgb skin = {248, 172, 134}, hollow = {228, 140, 116};
	const int nt = 14, ns = 10;
	auto point = [&](float t, float s, int face) {
		// its outline: a rounded foot, widest a quarter along, then a long curve in to the point
		float w = wide * (t < 0.25f ? 0.7f + 0.3f * sinf(t / 0.25f * kPi / 2) : powf(1 - (t - 0.25f) / 0.75f, 1.25f));
		float edge = 1 - s * s;                          // 0 at its edge, 1 down its middle
		float depth = face ? thick * 0.5f * sqrtf(edge) : -thick * 0.5f * sqrtf(edge);
		float dish  = face ? -cup * edge * sinf(kPi * (0.08f + 0.84f * t)) : 0.0f; // the front cupped in
		float bend  = 2.5f * t * t;                      // its point curling back a little
		return add(add(add(base, mul(L, len * t)), mul(W, w * s)), mul(N, depth + dish - bend));
	};
	for (int face = 0; face < 2; face++)
		for (int i = 0; i < nt; i++)
			for (int k = 0; k < ns; k++) {
				float t0 = (float)i / nt, t1 = (float)(i + 1) / nt, s0 = -1 + 2.0f * k / ns, s1 = -1 + 2.0f * (k + 1) / ns;
				V a = point(t0, s0, face), b = point(t0, s1, face), c = point(t1, s1, face), d = point(t1, s0, face);
				bool inHollow = face == 1 && fabsf((s0 + s1) / 2) < 0.55f && t0 > 0.1f && t1 < 0.85f;
				Rgb col = inHollow ? hollow : skin;
				// both faces outward: the front toward N, the back away from it
				bool flip = face == 0;
				if (flip)
					m.quad(a, b, c, d, col);
				else
					m.quad(a, d, c, b, col);
			}
}
void elfEars(Mesh& m)
{
	m.light = norm(v(0.2f, 0.9f, 0.4f));
	elfEar(m, 1);
	elfEar(m, -1);
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
// Santa's beard (the "santa_beard" option, Off): one soft, full mass from his sideburns close round his jaw,
// over his mouth and down to a rounded point, its locks waving, off-white and a little grey-blue in the hollows
// between them. A grid round his face (u from one ear to the other) and down (v from where it meets his face
// to its end), made twice, the inside on his face, so it is solid from every side.
// How far his face, neck and chest reach out from his middle line (x 0, z 3), all round (every 10 degrees from
// 100 on his right through his front to 100 on his left) at heights 60 to 100, every 4: found by casting rays
// at ma_mdl1 (his nose and ears left out)
const float kFaceR[21][11] = {
	{26.7f, 26.6f, 26.9f, 21.3f, 21.3f, 21.3f, 21.3f, 21.0f, 17.8f, 20.9f, 21.6f},
	{25.7f, 25.6f, 25.8f, 24.6f, 21.3f, 21.3f, 14.3f, 15.9f, 17.8f, 20.3f, 21.5f},
	{25.1f, 25.0f, 25.2f, 23.5f, 14.0f, 14.0f, 15.7f, 15.7f, 18.4f, 20.3f, 21.6f},
	{25.2f, 25.2f, 24.6f, 23.6f, 21.8f, 19.9f, 17.4f, 17.1f, 18.1f, 20.2f, 21.2f},
	{25.0f, 24.9f, 24.0f, 23.1f, 19.2f, 19.2f, 15.9f, 16.2f, 17.6f, 20.4f, 21.0f},
	{24.0f, 23.8f, 23.2f, 22.3f, 20.8f, 18.2f, 18.2f, 14.3f, 17.4f, 19.3f, 19.6f},
	{23.7f, 23.5f, 23.1f, 21.6f, 20.1f, 17.8f, 15.4f, 14.2f, 17.0f, 18.5f, 18.9f},
	{23.1f, 22.9f, 22.5f, 21.2f, 19.9f, 17.7f, 6.9f, 14.0f, 17.2f, 18.3f, 18.7f},
	{22.3f, 22.2f, 21.9f, 20.5f, 18.9f, 9.7f, 8.2f, 14.1f, 17.2f, 17.7f, 18.2f},
	{18.4f, 17.7f, 16.8f, 15.0f, 12.3f, 9.1f, 8.8f, 14.8f, 17.5f, 17.7f, 17.7f},
	{19.0f, 18.3f, 17.2f, 14.7f, 12.0f, 8.9f, 8.7f, 15.5f, 18.2f, 18.2f, 18.2f},
	{18.4f, 17.7f, 16.8f, 15.0f, 12.3f, 9.1f, 8.9f, 14.8f, 17.5f, 17.7f, 17.7f},
	{22.3f, 22.2f, 21.9f, 20.5f, 18.9f, 9.7f, 8.2f, 14.1f, 17.2f, 17.7f, 18.2f},
	{23.1f, 22.9f, 22.5f, 21.2f, 19.9f, 17.7f, 6.9f, 14.0f, 17.2f, 18.3f, 18.7f},
	{23.7f, 23.5f, 23.1f, 21.6f, 20.1f, 17.8f, 15.4f, 14.2f, 17.0f, 18.5f, 18.9f},
	{24.0f, 23.8f, 23.2f, 22.3f, 20.8f, 18.2f, 18.2f, 14.3f, 17.4f, 19.3f, 19.6f},
	{25.0f, 24.9f, 24.0f, 23.1f, 19.2f, 19.2f, 15.9f, 16.2f, 17.6f, 20.4f, 21.0f},
	{25.2f, 25.2f, 24.6f, 23.6f, 21.8f, 19.9f, 17.4f, 17.1f, 18.1f, 20.2f, 21.2f},
	{25.1f, 25.0f, 25.2f, 23.5f, 14.0f, 14.0f, 15.6f, 15.6f, 18.4f, 20.3f, 21.6f},
	{25.7f, 25.6f, 25.8f, 24.6f, 21.3f, 21.3f, 14.3f, 15.9f, 17.8f, 20.3f, 21.5f},
	{26.7f, 26.6f, 26.9f, 21.3f, 21.3f, 21.3f, 21.3f, 21.0f, 17.8f, 20.9f, 21.6f}
};
float faceRadius(float th, float y)
{
	float a = (th * 180 / kPi + 100) / 10, h = (y - 60) / 4;
	a = a < 0 ? 0 : (a > 20 ? 20 : a);
	h = h < 0 ? 0 : (h > 10 ? 10 : h);
	int a0 = (int)a, h0 = (int)h;
	int a1 = a0 < 20 ? a0 + 1 : 20, h1 = h0 < 10 ? h0 + 1 : 10;
	float fa = a - a0, fh = h - h0;
	float lo = kFaceR[a0][h0] * (1 - fa) + kFaceR[a1][h0] * fa, hi = kFaceR[a0][h1] * (1 - fa) + kFaceR[a1][h1] * fa;
	return lo * (1 - fh) + hi * fh;
}
// A mouth for him without his moustache (it hid where his mouth would be): closed in a gentle smile,
// lying on his face under his nose (a grid over the crescent, each point
// carried out to his face's surface)
V onFace(float x, float y, float out)
{
	float r = faceRadius(0, y) + out;
	float zz = r * r - x * x;
	return v(x, y, 3 + sqrtf(zz > 1 ? zz : 1.0f));
}
void crescent(Mesh& m, float w, float yTop, float sag, float lift, float yBottom, float out, Rgb col)
{
	// top edge: a smile, its corners `lift` up; bottom edge: `yBottom` deep in the middle
	const int nx = 14, ny = 4;
	for (int i = 0; i < nx; i++)
		for (int k = 0; k < ny; k++) {
			V q[4];
			const int di[4] = {0, 1, 1, 0}, dk[4] = {0, 0, 1, 1};
			for (int c = 0; c < 4; c++) {
				float sx = -1 + 2.0f * (i + di[c]) / nx, t = (float)(k + dk[c]) / ny;
				float x = sx * w, top = yTop - sag * (1 - sx * sx) + lift * sx * sx * sx * sx;
				float bot = top - yBottom * (1 - sx * sx);
				q[c] = onFace(x, top + (bot - top) * t, out);
			}
			m.quad(q[0], q[3], q[2], q[1], col);
		}
}
void marioMouth(Mesh& m)
{
	m.light = norm(v(0, 0.3f, 1));
	// closed, a soft smile: a thin line of the lips meeting, its corners a little up, and the lower lip's
	// pink just under it, higher on his face (under his nose, not at his chin)
	crescent(m, 6.8f, 93.4f, 0.7f, 0.9f, 1.0f, 0.45f, Rgb{132, 52, 50});
	crescent(m, 4.6f, 92.3f, 0.45f, 0.2f, 1.2f, 0.42f, Rgb{232, 140, 126});
	intoFrame(m, kMarioHead);
}

// a point of the beard: its inside (outer 0) on his face, a hair out; its outside thicker in the middle of
// its length and waving in locks
V beardPoint(float u, float vv, float outer)
{
	float th   = u * 1.72f, co = cosf(th), front = co > 0 ? co : 0.0f;
	float top  = 94.0f + 7.0f * (1 - co);              // from the skin under his nose, up to his sideburns
	float len  = 10 + 22 * powf(front, 1.4f);          // longer in front, short by his ears
	float y    = top - vv * len;
	float lock = 1.0f * sinf(u * 26) * (0.35f + vv) + 0.7f * sinf(u * 11 + vv * 7) + 0.5f * sinf(vv * 13 + u * 5);
	float thick = outer * (3.0f + 6.5f * sinf(kPi * (0.1f + 0.8f * vv)) + lock * 0.6f); // fullest below his chin
	thick *= 1 - 0.3f * vv * vv * vv * front;          // rounding off at its point
	// its inside on his skin, and below his chin hanging straight down from it (not in under it to his neck)
	float skin = faceRadius(th, y);
	if (y < 86) {
		float hang = faceRadius(th, 86) - (86 - y) * 0.12f;
		skin = skin > hang ? skin : hang;
	}
	float r    = skin - 0.3f + (thick > 0 ? thick : 0.0f);
	return v(r * sinf(th), y, 3 + r * co + 1.5f * vv * vv * front); // the point a hair forward, on his chest
}
void santaBeard(Mesh& m)
{
	m.light = norm(v(0.2f, 0.7f, 0.7f));
	const int nu = 44, nv = 14;
	for (int layer = 0; layer < 2; layer++) {
		float depth = layer ? 0.0f : 1.0f; // the outside, then the inside against his face
		for (int i = 0; i < nu; i++)
			for (int k = 0; k < nv; k++) {
				float u0 = -1 + 2.0f * i / nu, u1 = -1 + 2.0f * (i + 1) / nu;
				float v0 = (float)k / nv, v1 = (float)(k + 1) / nv;
				V a = beardPoint(u0, v0, depth), b = beardPoint(u1, v0, depth), c = beardPoint(u1, v1, depth),
				  d = beardPoint(u0, v1, depth);
				float groove = 0.5f + 0.5f * sinf((u0 + u1) * 13);   // between locks: a touch darker, bluer
				Rgb col = {(uint8_t)(248 - 26 * groove), (uint8_t)(248 - 22 * groove), (uint8_t)(250 - 12 * groove)};
				if (layer == 0)
					m.quad(a, d, c, b, col);
				else
					m.quad(a, b, c, d, Rgb{214, 218, 228});
			}
	}
	intoFrame(m, kMarioHead);
}
// a white moustache with the beard: two soft locks lying on the skin under his nose (from the middle of it,
// at 95 high, out along his face to the corners of his mouth, curling down there), so it sits under his nose
// and not out in front of it
void santaMoustache(Mesh& m)
{
	m.light = norm(v(0.2f, 0.7f, 0.7f));
	for (int s = -1; s <= 1; s += 2) {
		std::vector<V> c;
		std::vector<float> r;
		for (int i = 0; i <= 12; i++) {
			float t = i / 12.0f, rad = 4.2f * (1 - t) + 1.4f * t;
			float x = s * (1 + 15 * t), y = 95 - 1.0f * t - 6 * t * t * t;
			c.push_back(onFace(x, y, rad * 0.75f));
			r.push_back(rad);
		}
		m.tube(c, r, kFur, 10);
	}
	intoFrame(m, kMarioHead);
}

// His own moustache, gone with the "no_moustache" option or under the beard: it is a piece of its own in his face (ma_mdl1's position
// array, these 31 vertices, on his head joint, as the file has them), folded into one point inside his head,
// and put back after. They are checked first, so a changed model (another character) is left alone.
struct MoustacheVertex {
	int index;
	float at[3];
};
const MoustacheVertex kMoustache[] = {
	{40, {20.1406f, -3.8789f, 16.7422f}},
	{41, {23.2852f, -4.6367f, 13.6523f}},
	{42, {21.3984f, -3.1406f, 14.3281f}},
	{43, {22.3984f, -7.8672f, 12.6328f}},
	{45, {18.9531f, -6.4609f, 17.5117f}},
	{97, {24.6484f, -6.0742f, -7.8008f}},
	{98, {25.6094f, -5.5273f, -4.5781f}},
	{99, {23.9219f, 1.5391f, -7.4609f}},
	{100, {23.4336f, 1.4570f, -9.8945f}},
	{101, {24.2852f, 3.1328f, -5.1953f}},
	{102, {27.0703f, -0.9453f, -5.9688f}},
	{103, {20.1406f, -3.8789f, -16.7422f}},
	{104, {23.2852f, -4.6367f, -13.6523f}},
	{105, {18.9531f, -6.4609f, -17.5117f}},
	{106, {22.3984f, -7.8672f, -12.6328f}},
	{108, {21.3984f, -3.1406f, -14.3281f}},
	{109, {23.6797f, -6.9023f, -10.4688f}},
	{110, {21.9805f, 0.5430f, -12.4492f}},
	{125, {24.2852f, 3.1328f, 5.1953f}},
	{126, {24.7422f, 3.8320f, 2.1875f}},
	{127, {25.6094f, -5.5273f, 4.5781f}},
	{128, {24.7422f, 3.8320f, -2.1875f}},
	{129, {27.0703f, -0.9453f, 5.9688f}},
	{132, {23.9219f, 1.5391f, 7.4609f}},
	{133, {23.4336f, 1.4570f, 9.8945f}},
	{134, {21.9805f, 0.5430f, 12.4492f}},
	{135, {24.6484f, -6.0742f, 7.8008f}},
	{158, {23.6797f, -6.9023f, 10.4688f}},
	{173, {28.3789f, -0.9844f, 0.0000f}},
	{175, {26.7109f, -5.3477f, 0.0000f}},
	{186, {25.1953f, 2.9727f, 0.0000f}},
};
const int kMoustacheCount = (int)(sizeof kMoustache / sizeof kMoustache[0]);
const float kInsideHead[3] = {15, 0, 0}; // (0, 97, 12) in the model: behind his upper lip

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

// A snowball, 10 across at size 1 (scaled to each one's drop): lumpy, white, a little blue underneath
void snowball(Mesh& m)
{
	m.light = norm(v(0.3f, 0.9f, 0.3f));
	const int seg = 12, rings = 8;
	for (int i = 0; i < rings; i++)
		for (int k = 0; k < seg; k++) {
			V p[4];
			for (int c = 0; c < 4; c++) {
				int ii = i + (c >= 2), kk = k + (c == 1 || c == 2);
				float a = kPi * ii / rings - kPi / 2, b = 2 * kPi * kk / seg;
				float r = 5 + 0.45f * sinf(a * 5 + b * 3) + 0.3f * cosf(b * 5 - a * 2); // lumps
				if (ii == 0 || ii == rings) r = 5;
				p[c] = v(r * cosf(a) * cosf(b), r * sinf(a), r * cosf(a) * sinf(b));
			}
			Rgb col = i < 2 ? Rgb{214, 224, 240} : Rgb{248, 249, 252};
			m.quad(p[0], p[1], p[2], p[3], col);
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
	SmsMesh sHat = 0, sBelt = 0, sCuffR = 0, sCuffL = 0, sLegR = 0, sLegL = 0, sBeard = 0, sTache = 0, fluddDeco = 0;
	SmsMesh sMouth = 0, ball = 0, eHat = 0, eEars = 0;
	int suitWorn = 0; // the costume whose colours are on him: 0 none, 1 Santa, 2 elf
	float lastSnowball[2] = {-9, -9}; // when each player last threw one
	bool tacheGone = false, tacheLeft = false; // his own moustache folded away; or never touched (not the game's model)
	bool capOff = false; // his own cap left out
	bool santaOn = false, fluddOn = false; // tints put on
	bool rudolphOn = false; // Yoshi's colour set
	std::set<SmsObject> dressed, snowy; // Piantas' clothes and snowmen tinted, this stage
	std::set<SmsObject> bare;           // Piantas whose own hat and leaf hair are off under the Santa hat, this stage
	// gingerbread men: where each started (the level's marker), where he is, which way he faces, where he goes
	struct Ginger {
		V home, at, target;
		float yaw = 0, phase = 0, wait = 0, wave = 0, speed = 0;
		float cool = 0, swing = 0, gone = 0; // till he can swing again; his swing; crumbled, back in so long
		float wet = 0, hurt = 0, dying = 0;  // seconds of water on him (3 melts him); trembling; his end so far
		int dieKind = 0;                     // 0 jumped on (falls apart), 1 melted
		bool chasing = false;
		unsigned seed = 1;
	};
	std::vector<Ginger> gingers;
	int gingerLooks = 0;
	float gingerRest = 0; // after any of them hits Mario: none swings again till this runs out
	double gingerClock = -1;
	SmsMesh gBody = 0, gArm[2] = {}, gLeg[2] = {};
	std::set<SmsObject> baubled;        // Bob-omb baubles tinted, this stage
	std::vector<std::pair<SmsObject, int> > baubles; // the level's Bob-omb baubles and their numbers, this stage
	int baubleLooks = 0;
	SmsMesh baubleCapMesh = 0, baublePat[5][4] = {};
	bool baubleLogged = false;
	std::set<SmsObject> wasUnder;       // Stu balls that have had another on top of them, this stage: never a head
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

	// A Pianta's own hat (a straw hat, a woman's flowered one: worn models "hat..." and "hw...") left off
	// and its palm-leaf hair hidden, so the Santa hat sits alone; or both back
	static bool ownHat(const char* name) { return name && (!strncmp(name, "hat", 3) || !strncmp(name, "hw", 2)); }
	void bareHead(SmsObject o, int off)
	{
		static const char* const kHair[] = {"yashi_jnt", "leafL_jnt", "leafR_jnt"};
		for (int k = 0; k < 3; k++)
			parts->hide_joint(o, kHair[k], off);
		for (int i = 0; i < 12; i++)
			if (ownHat(parts->worn_name(o, i)))
				parts->hide_worn(o, i, off);
	}
	void unbare()
	{
		for (std::set<SmsObject>::iterator i = bare.begin(); i != bare.end(); ++i)
			bareHead(*i, 0); // Piantas stay all stage long
		bare.clear();
	}

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

	// his own moustache folded away (gone) or put back
	void shaveMoustache(SmsObject mario, bool gone)
	{
		if (gone == tacheGone || tacheLeft || !partsHave(offsetof(SmsPartsApi, set_vertex) + sizeof(void*)))
			return;
		if (gone) {
			for (int i = 0; i < kMoustacheCount; i++) { // the model as the file has it?
				float p[3];
				if (!parts->vertex(mario, kMoustache[i].index, p) || fabsf(p[0] - kMoustache[i].at[0]) > 0.02f
				    || fabsf(p[1] - kMoustache[i].at[1]) > 0.02f || fabsf(p[2] - kMoustache[i].at[2]) > 0.02f) {
					warn("Mario's model is not the game's own: his moustache is left as it is");
					tacheLeft = true;
					return;
				}
			}
			for (int i = 0; i < kMoustacheCount; i++)
				parts->set_vertex(mario, kMoustache[i].index, kInsideHead);
		} else
			for (int i = 0; i < kMoustacheCount; i++)
				parts->set_vertex(mario, kMoustache[i].index, kMoustache[i].at);
		tacheGone = gone;
	}

	// Santa Mario: his clothes Santa red, his shoes black boots, a Santa hat, a belt, fur cuffs (and a beard)
	// Christmas FLUDD: red and green, with gold handles, wrapped in a gold ribbon with a bow and holly
	void santa()
	{
		if (!partsHave(offsetof(SmsPartsApi, yoshi_color) + sizeof(void*)))
			return;
		const int costume = config().get_int(handle(), "costume", 1); // 0 none, 1 Santa, 2 an elf
		const bool suit   = costume == 1 || costume == 2, elf = costume == 2;
		const bool xf   = config().get_int(handle(), "christmas_fludd", 1) != 0;
		SmsObject mario = parts->player_part(SMS_PARTS_MARIO);
		SmsObject fludd = parts->player_part(SMS_PARTS_FLUDD);
		if (mario && (suit || santaOn || suitWorn != costume)) {
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
				const SmsColor cloth = elf ? 0x1E8238F0u : 0xB8121CF0u; // an elf's green, Santa's red
				parts->tint(mario, "_mat_head", cloth);
				parts->tint(mario, "_mat_head(", 0);
				parts->tint(mario, "_mat_head(2)", cloth);
				parts->tint(mario, "_mat_head(3)", 0x1C1A1EE6u);
				parts->tint(mario, "_mat_head(4)", 0x1C1A1EE6u);
			} else
				parts->tint(mario, "", 0);
			santaOn  = suit;
			suitWorn = costume;
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
		// his own moustache: gone with "No Mario moustache", and under Santa's beard (a white one over it)
		const bool noTache = config().get_int(handle(), "no_moustache", 0) != 0;
		if (mario)
			shaveMoustache(mario, noTache || (suit && config().get_int(handle(), "santa_beard", 0) != 0));
		// with his moustache gone (and his model the game's own), a mouth where it was
		if (mario && tacheGone && !tacheLeft && !firstPerson())
			drawOn(mario, kFaceJoint, sMouth);
		if (mario && suit) {
			if (!firstPerson()) {
				drawOn(mario, kFaceJoint, elf ? eHat : sHat);
				if (elf)
					drawOn(mario, kFaceJoint, eEars);
				if (config().get_int(handle(), "santa_beard", 0)) {
					// his own moustache goes with the beard (shaveMoustache); a white one unless he is clean shaven
					drawOn(mario, kFaceJoint, sBeard);
					if (!noTache)
						drawOn(mario, kFaceJoint, sTache);
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

	// FLUDD throws snowballs ("snowball_fludd", Off): its spray nozzle's stream is turned down, and a fast
	// snowball made in its place every so often, from the same place and the same way but faster and
	// bigger, drawn by the mod (the game's drop unseen under it), bursting into snow where it lands. It
	// still hits as the game's water does, and uses the tank's water.
	static const uint32_t kSnowTag = 0x534E0000u; // "SN"
	bool snowballsOn()
	{
		return config().get_int(handle(), "snowball_fludd", 0) != 0 && SMS_API_HAS(&api(), water)
		       && SMS_API_HAS(&api(), models);
	}
	int onWaterEmit(SmsEvWaterEmit& e)
	{
		if (!snowballsOn() || e.player < 0 || e.player > 1)
			return SMS_CONTINUE;
		SmsPlayer p;
		memset(&p, 0, sizeof p);
		p.size = sizeof p;
		if (!player().get || !player().get(&p) || !p.valid || p.current_nozzle != 0 || p.in_water)
			return SMS_CONTINUE; // the underwater nozzle, or under water: its own water
		if (now - lastSnowball[e.player] < 0.16f)
			return SMS_CANCEL;
		lastSnowball[e.player] = now;
		SmsWaterDrop d = e.drop;
		d.size = sizeof d;
		d.velocity.x *= 1.9f, d.velocity.y *= 1.9f, d.velocity.z *= 1.9f;
		d.velocity.y += 2.0f;            // a little lob, so it carries
		d.drop_size  = d.drop_size * 1.5f + 2;
		d.hit_radius = d.hit_radius * 1.6f + 10;
		d.hit_height = d.hit_height * 1.6f + 10;
		d.alive      = d.alive * 2 + 20;
		d.attack     = d.attack > 0 ? d.attack : 1;
		d.tag        = kSnowTag | (uint32_t)e.player;
		d.unseen     = 1;
		d.player     = e.player;
		if (api().water->emit(&d) >= 0)
			api().water->use(e.player, 6);
		return SMS_CANCEL;
	}
	int onWaterLand(SmsEvWaterLand& e)
	{
		if ((e.tag & 0xFFFF0000u) != kSnowTag)
			return SMS_CONTINUE;
		V at = v(e.position.x, e.position.y, e.position.z);
		for (int i = 0; i < 5; i++) {
			float a = 2 * kPi * i / 5;
			addPuff(add(at, v(cosf(a) * 14, 6 + 4 * (i & 1), sinf(a) * 14)), 24);
		}
		addPuff(add(at, v(0, 10, 0)), 34);
		return SMS_CANCEL; // it bursts: no rolling on as water
	}
	void drawSnowballs()
	{
		if (!ball || !SMS_API_HAS(&api(), water))
			return;
		const int n = api().water->count();
		for (int i = 0; i < n; i++) {
			SmsWaterDrop d;
			memset(&d, 0, sizeof d);
			d.size = sizeof d;
			if (!api().water->get(i, &d) || (d.tag & 0xFFFF0000u) != kSnowTag)
				continue;
			const float sc = (d.drop_size > 1 ? d.drop_size : 8) / 10;
			const float m[3][4] = {{sc, 0, 0, d.position.x}, {0, sc, 0, d.position.y}, {0, 0, sc, d.position.z}};
			draw(ball, m);
		}
	}

	// The FLUDD Weapons mod's black goop as ice (its "fludd-weapons.goop" service): asked for while the option
	// is on, given back as black goop when it is turned off or this mod unloads
	struct FluddWeaponsGoop {
		uint32_t size;
		void (*set_ice)(int on);
		int (*ice)(void);
	};
	bool iceAsked = false, iceKnown = false; // what FLUDD Weapons was last told; whether that is still so
	void askIce(bool on)
	{
		iceKnown = true;
		const SmsCoreApi* core = api().core;
		if (!core->is_loaded("fludd-weapons"))
			return;
		const FluddWeaponsGoop* g = (const FluddWeaponsGoop*)core->require(handle(), "fludd-weapons.goop", 1);
		if (!g || g->size < offsetof(FluddWeaponsGoop, set_ice) + sizeof(void*) || !g->set_ice)
			return;
		g->set_ice(on ? 1 : 0);
		if (on != iceAsked)
			info(on ? "FLUDD Weapons' black goop is ice" : "FLUDD Weapons' black goop is black goop again");
		iceAsked = on;
	}
	~ChristmasAdditions()
	{
		if (iceAsked)
			askIce(false);
	}

	// Gingerbread men: found once a stage (its markers), then each wanders round where he started (a spot
	// picked within 7 m, walked to, a pause, the next). Mario within 12 m: he runs after him, and close enough
	// he swings at him (a point of health and a knock back, then a moment before he can again). Jumped on,
	// he crumbles, and is back where he started half a minute later. Mario far off again: he walks home.
	static float frand(unsigned& s)
	{
		s = s * 1103515245u + 12345u;
		return ((s >> 8) & 0xFFFF) / 65535.0f;
	}
	// a step his way, kept out of walls and on the ground: false when a wall stopped him
	bool gingerStep(Ginger& g, float tx, float tz, float speed, float turn, float dt)
	{
		float dx = tx - g.at.x, dz = tz - g.at.z;
		float want = atan2f(dx, dz), dy = want - g.yaw;
		while (dy > kPi) dy -= 2 * kPi;
		while (dy < -kPi) dy += 2 * kPi;
		g.yaw += dy * (dt * turn < 1 ? dt * turn : 1);
		if (fabsf(dy) > 1.0f)
			speed *= 0.3f;
		g.speed = speed;
		SmsVec3 want3 = {g.at.x + sinf(g.yaw) * speed * dt, g.at.y + 60, g.at.z + cosf(g.yaw) * speed * dt}, out = want3;
		if (game().out_of_walls)
			game().out_of_walls(&want3, 40.0f, &out, nullptr);
		const float moved = (out.x - g.at.x) * (out.x - g.at.x) + (out.z - g.at.z) * (out.z - g.at.z);
		g.at.x = out.x, g.at.z = out.z;
		SmsGround gr;
		memset(&gr, 0, sizeof gr);
		gr.size = sizeof gr;
		SmsVec3 probe = {g.at.x, g.at.y + 120, g.at.z};
		if (game().ground && game().ground(&probe, &gr) && fabsf(gr.floor - g.at.y) < 200)
			g.at.y = gr.floor;
		return moved >= (speed * dt * 0.3f) * (speed * dt * 0.3f);
	}
	void gingerbread(const SmsPlayer& p)
	{
		const double t = api().core->time();
		float dt = gingerClock < 0 ? 0.0f : (float)(t - gingerClock);
		gingerClock = t;
		if (dt > 0.1f)
			dt = 0.1f;
		if (gingers.empty() && gingerLooks < 8 && game().find_object && game().object_position) {
			gingerLooks++;
			for (int i = 0; i < 32; i++) {
				char name[24];
				snprintf(name, sizeof name, "Gingerbread %d", i);
				SmsObject o = game().find_object(name);
				SmsVec3 at;
				if (!o || !game().object_position(o, &at))
					continue;
				Ginger g;
				g.home = g.at = g.target = v(at.x, at.y, at.z);
				g.seed = 977u * (unsigned)(i + 1);
				g.yaw  = frand(g.seed) * 2 * kPi;
				g.wait = frand(g.seed) * 2;
				gingers.push_back(g);
			}
			if (!gingers.empty()) {
				gingerLooks = 99;
				info("%d gingerbread men", (int)gingers.size());
			}
		}
		const bool fierce  = setting("gingerbread_chase", 1) != 0;
		gingerRest -= dt;
		const bool canHurt = SMS_API_HAS(&api(), player) && player().size >= offsetof(SmsPlayerApi, hurt) + sizeof(void*) && player().hurt;
		for (size_t i = 0; i < gingers.size(); i++) {
			Ginger& g = gingers[i];
			if (g.gone > 0) { // crumbled: back home after a while
				g.gone -= dt;
				if (g.gone <= 0)
					g.at = g.target = g.home, g.chasing = false, g.wait = 1, g.wet = 0;
				continue;
			}
			if (g.dying > 0) { // his end: drawn as it goes, then gone
				g.dying += dt;
				if (g.dying >= 1.2f)
					g.dying = 0, g.gone = 30;
				else
					drawGinger(g, 0);
				continue;
			}
			// FLUDD's water on him: the game's flying drops passing through him (each used up on him)
			bool hitNow = false;
			const SmsWaterApi* wa = SMS_API_HAS(&api(), water) ? api().water : nullptr;
			if (wa && wa->count && wa->get) {
				const int nd = wa->count();
				for (int k = 0; k < nd; k++) {
					SmsWaterDrop wd;
					memset(&wd, 0, sizeof wd);
					wd.size = sizeof wd;
					if (!wa->get(k, &wd) || wd.landed || wd.tag != 0)
						continue;
					const float ddx = wd.position.x - g.at.x, ddz = wd.position.z - g.at.z, ddy = wd.position.y - g.at.y;
					if (ddx * ddx + ddz * ddz < 55.0f * 55.0f && ddy > -10 && ddy < 230) {
						hitNow = true;
						wd.alive = 0;
						if (wa->set)
							wa->set(k, &wd);
					}
				}
			}
			if (hitNow) {
				g.wet += dt * 2.0f; // (the stream reaches him a tick in two)
				g.hurt = 0.25f;
				g.chasing = true;   // and he is cross
				if (g.wet >= 3.0f) {
					g.dying = 0.001f, g.dieKind = 1;
					info("a gingerbread man melted");
					continue;
				}
			}
			g.hurt -= dt;
			g.cool -= dt, g.swing -= dt;
			const float mdx = p.position.x - g.at.x, mdz = p.position.z - g.at.z, mdy = p.position.y - g.at.y;
			const float d2 = mdx * mdx + mdz * mdz;
			const float hx = g.at.x - g.home.x, hz = g.at.z - g.home.z;
			// jumped on: Mario coming down onto his head
			if (fierce && d2 < 75.0f * 75.0f && mdy > 110 && mdy < 270 && p.velocity.y < 0) {
				g.dying = 0.001f, g.dieKind = 0;
				addPuff(add(g.at, v(0, 90, 0)), 50);
				SmsVec3 up = p.velocity;
				up.y = 28;
				player().set_velocity(&up);
				continue;
			}
			if (!g.chasing && fierce && d2 < 1200.0f * 1200.0f && fabsf(mdy) < 400)
				g.chasing = true;
			if (g.chasing && (!fierce || d2 > 1700.0f * 1700.0f || hx * hx + hz * hz > 2500.0f * 2500.0f || fabsf(mdy) > 700)) {
				g.chasing = false; // too far from Mario, or from home: back
				g.target  = g.home;
				g.wait    = 0;
			}
			g.speed = 0;
			if (g.chasing) {
				if (g.hurt > 0)
					gingerStep(g, p.position.x, p.position.z, 0.0f, 3.0f, dt); // held back by the water
				else if (d2 > 75.0f * 75.0f)
					gingerStep(g, p.position.x, p.position.z, 330.0f, 6.0f, dt);
				else
					gingerStep(g, p.position.x, p.position.z, 0.0f, 8.0f, dt);
				// close: a swing at him (his right arm), once in a while
				// Not while he is down, or getting up: a hit gives every gingerbread man a rest longer
				// than Mario takes to get back on his feet (Allebale: they pummelled him to death)
				const bool down = (p.status & 0xFFFF0u) == 0x208B0u; // the knocked-down falls (0x208B0..0x208BF)
				if (d2 < 115.0f * 115.0f && fabsf(mdy) < 120 && g.cool <= 0 && gingerRest <= 0 && !down) {
					g.cool     = 5.0f;
					gingerRest = 4.0f;
					g.swing    = 0.35f;
					if (canHurt && !p.invincible) {
						// a point of health, then knocked back off his feet, away from him (the game's own
						// knocked-back fall, as a wall hit too hard gives), facing him
						player().hurt(1);
						if (player().set_status && player().set_yaw)
							player().set_yaw(atan2f(-mdx, -mdz) * 57.29578f);
						if (player().set_status)
							player().set_status(0x208B3, 0);
						if (player().set_forward_speed)
							player().set_forward_speed(-24.0f);
						SmsVec3 kb = p.velocity;
						kb.y = 32.0f;
						player().set_velocity(&kb);
					}
				}
			} else if (g.wait > 0) {
				g.wait -= dt;
			} else {
				float dx = g.target.x - g.at.x, dz = g.target.z - g.at.z;
				if (dx * dx + dz * dz < 30 * 30) { // there: a pause, and somewhere else
					g.wait = 0.8f + frand(g.seed) * 2.5f;
					float a = frand(g.seed) * 2 * kPi, r = 150 + frand(g.seed) * 550;
					g.target = v(g.home.x + r * sinf(a), g.home.y, g.home.z + r * cosf(a));
				} else if (!gingerStep(g, g.target.x, g.target.z, 200.0f, 4.0f, dt)) {
					g.target = g.home; // a wall in the way: somewhere else
					g.wait   = 0.5f;
				}
			}
			g.phase += dt * g.speed * 0.045f;
			drawGinger(g, g.speed);
		}
	}
	// one gingerbread man drawn: walking (his limbs swinging), hit by water (trembling, melting lower and wider
	// as he soaks), jumped on (squashed, then his pieces flying apart and falling) or melted away
	void drawGinger(const Ginger& g, float speed)
	{
		float sy = 1.0f, sw = 1.0f;                         // his height and width, times his own
		const float thin = 0.62f;                           // how thick a cookie is
		const float soak = g.wet / 3.0f;                    // 0 dry, 1 melted
		sy -= 0.25f * soak, sw += 0.12f * soak;
		float shake = g.hurt > 0 ? 4.0f * sinf(g.hurt * 70.0f) : 0.0f;
		float burst = -1;                                   // jumped on: 0..1 through his falling apart
		if (g.dying > 0 && g.dieKind == 1) {                // melted: down into a puddle of dough
			const float k = g.dying / 1.2f;
			sy *= 1.0f - 0.92f * k, sw *= 1.0f + 0.9f * k, shake = 0;
		} else if (g.dying > 0) {
			if (g.dying < 0.15f)
				sy *= 1.0f - 2.8f * g.dying, sw *= 1.0f + 2.0f * g.dying; // squashed flat first
			else
				burst = (g.dying - 0.15f) / 1.05f, sy *= 0.58f, sw *= 1.3f;
		}
		const V right = v(cosf(g.yaw), 0, -sinf(g.yaw)), up = v(0, 1, 0), fwd = v(sinf(g.yaw), 0, cosf(g.yaw));
		const float swing = speed > 1 ? 0.55f * sinf(g.phase) : 0.0f;
		const float bob = speed > 1 ? 4.0f * fabsf(sinf(g.phase)) : 0.0f;
		// a piece's frame: its own x, y, z turned about x (a swing forward and back) and z (out to the side)
		auto frame = [&](float m[3][4], V at, float swingX, float outZ) {
			float cx = cosf(swingX), sx = sinf(swingX), cz = cosf(outZ), sz = sinf(outZ);
			V ax = add(mul(right, cz), mul(up, sz));
			V ay = add(mul(up, cz * cx), add(mul(right, -sz * cx), mul(fwd, sx)));
			V az = sub(mul(fwd, cx), mul(add(mul(up, cz), mul(right, -sz)), sx));
			ax = mul(ax, sw), ay = mul(ay, sy), az = mul(az, sw * thin);
			m[0][0] = ax.x, m[0][1] = ay.x, m[0][2] = az.x, m[0][3] = at.x;
			m[1][0] = ax.y, m[1][1] = ay.y, m[1][2] = az.y, m[1][3] = at.y;
			m[2][0] = ax.z, m[2][1] = ay.z, m[2][2] = az.z, m[2][3] = at.z;
		};
		// flying apart: a piece pushed out its way and up, falling, tumbling
		auto fly = [&](V at, V way, float spin, float& swingX, float& outZ) -> V {
			if (burst < 0)
				return at;
			const float t = burst * 1.05f;
			V o = add(mul(way, 260.0f * t), v(0, 330.0f * t - 900.0f * t * t, 0));
			if (o.y < -at.y + g.at.y + 5)
				o.y = -at.y + g.at.y + 5; // not through the ground
			swingX += spin * t * 9, outZ += spin * t * 5;
			return add(at, o);
		};
		float m[3][4];
		const V base = add(add(g.at, v(0, bob, 0)), mul(right, shake));
		float bx = 0, bz = 0;
		frame(m, fly(base, v(0, 0.3f, 0), 0.4f, bx, bz), bx, bz);
		draw(gBody, m);
		for (int side = 0; side < 2; side++) {
			const float sgn = side ? 1.0f : -1.0f;
			V hip = add(base, add(mul(right, kHipX * sgn * sw), mul(up, kHipY * sy)));
			float lx = swing * (side ? 1 : -1), lz = 0;
			V hipAt = fly(hip, add(mul(right, sgn * 0.8f), mul(fwd, 0.3f)), sgn, lx, lz);
			frame(m, hipAt, lx, lz);
			draw(gLeg[side], m);
			V shoulder = add(base, add(mul(right, kShoulderX * sgn * sw), mul(up, kShoulderY * sy)));
			float armSwing = -swing * 0.8f * (side ? 1 : -1), armOut = 0;
			if (g.swing > 0 && side == 1) // his right arm swung at Mario: up and forward, then down
				armSwing = -2.4f * (g.swing / 0.35f), armOut = 0.3f;
			if (g.hurt > 0) // flinching from the water: arms up over him
				armSwing = -2.0f, armOut = 0.4f * sgn;
			V shAt = fly(shoulder, add(mul(right, sgn * 1.0f), mul(fwd, -0.4f)), -sgn, armSwing, armOut);
			frame(m, shAt, armSwing, armOut);
			draw(gArm[side], m);
		}
	}

	void tick()
	{
		{
			const bool want = setting("ice_goop", 1) != 0;
			if (!iceKnown || want != iceAsked)
				askIce(want);
		}
		{
			SmsCamera cam;
			memset(&cam, 0, sizeof cam);
			cam.size = sizeof cam;
			bool camOk = SMS_API_HAS(&api(), camera) && api().camera->get(&cam) && cam.valid;
			rudolph(camOk ? &cam : nullptr);
			santa();
			drawSnowballs();
		}
		const bool hats = config().get_int(handle(), "santa_hats", 1) != 0;
		const bool clothes = config().get_int(handle(), "winter_clothes", 1) != 0;
		const bool snowmen = config().get_int(handle(), "snowmen", 1) != 0;
		if (!clothes && !dressed.empty())
			untint(dressed, true); // switched off
		const bool canBare = partsHave(offsetof(SmsPartsApi, hide_worn) + sizeof(void*));
		if (!hats && !bare.empty() && canBare)
			unbare();
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
				if (hats) {
					if (canBare && !bare.count(o)) {
						bareHead(o, 1);
						bare.insert(o);
					}
					draw(hat, m);
				}
				if (clothes && !dressed.count(o)) {
					unsigned pick = (unsigned)(m[0][3] * 0.37f + m[2][3] * 0.61f);
					parts->tint(o, "fuku", kWinter[pick % (sizeof kWinter / sizeof kWinter[0])]);
					dressed.insert(o);
				}
			}
		}
		if (setting("gingerbread", 1))
			gingerbread(p);

		// Bob-omb baubles: each its own colour (the whole Bob-omb tinted, shading kept), a cap and loop on top
		// and one of five patterns, picked by its number
		if (setting("baubles", 1)) {
			// found by name once a stage (as the gingerbread men's markers are: near() with the small enemy
			// flag never listed them)
			if (baubles.empty() && baubleLooks < 8 && game().find_object) {
				baubleLooks++;
				for (int i = 0; i < 64; i++) {
					char name[24];
					snprintf(name, sizeof name, "Bauble %d", i);
					if (SmsObject o = game().find_object(name))
						baubles.push_back(std::make_pair(o, i));
				}
				if (!baubles.empty()) {
					baubleLooks = 99;
					info("%d Bob-omb baubles", (int)baubles.size());
				}
			}
			static const SmsColor kBody[8] = {0xC81E28E0u, 0x2F5FD0E0u, 0x23964BE0u, 0xE6B43CE0u, 0x8C3CB4E0u, 0xD2D7E1E0u, 0xE65AA0E0u, 0x2AAAB4E0u};
			for (size_t i = 0; i < baubles.size(); i++) {
				SmsObject o = baubles[i].first;
				const int k = baubles[i].second;
				if (api().actor->flags && (api().actor->flags(o) & (SMS_ACTOR_FLAG_DEAD | SMS_ACTOR_FLAG_HIDDEN)))
					continue;
				if (!baubled.count(o) && game().object_set_tint) {
					game().object_set_tint(o, kBody[k % 8]);
					baubled.insert(o);
				}
				float rad = 0, tall = 0;
				if (!api().actor->hit_size || !api().actor->hit_size(o, &rad, &tall) || rad <= 0)
					continue;
				SmsVec3 at;
				if (!game().object_position || !game().object_position(o, &at))
					continue;
				if (!baubleLogged) { // what the body looks like to this mod, for fitting it better
					baubleLogged = true;
					info("bauble Bob-omb: hit radius %.0f, height %.0f; %d materials, %d joints", rad, tall,
					     parts->material_count(o), parts->joint_count(o));
					for (int j = 0; j < parts->material_count(o); j++)
						info("  material %s", parts->material_name(o, j));
					for (int j = 0; j < parts->joint_count(o); j++) {
						float jm[3][4];
						if (parts->joint(o, parts->joint_name(o, j), jm))
							info("  joint %s at %.0f %.0f %.0f", parts->joint_name(o, j), jm[0][3] - at.x, jm[1][3] - at.y, jm[2][3] - at.z);
					}
				}
				const float R = rad, cy = at.y + tall * 0.5f;
				const float m[3][4] = {{0, 0, R, at.x}, {R, 0, 0, cy}, {0, R, 0, at.z}}; // x up, y along world z, z along world x
				draw(baubleCapMesh, m);
				draw(baublePat[(k / 8 + k) % 5][(k * 3) % 4], m);
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
				// The face and hat are the head's: a ball on top with nothing ever stacked on it. Once a ball has
				// had one on top it is body for good, so with the head knocked off the balls left stay a
				// headless body (Allebale: each had grown a face and hat). Balls are not stacked yet when they
				// first come into view, so this goes by "ever", not by how they were first seen.
				if (above)
					wasUnder.insert(b.obj);
				if (!above && !wasUnder.count(b.obj))
					draw(head, b.m);
				else if (!above)
					draw(buttons, b.m);
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
		optionChoice("costume", "Mario's costume", {"His own clothes", "Santa", "Elf"}, 1);
		optionBool("santa_beard", "A white beard with Mario's costume", false);
		optionBool("no_moustache", "No Mario moustache", false);
		optionBool("christmas_fludd", "FLUDD wrapped for Christmas", true);
		optionBool("snowball_fludd", "FLUDD throws snowballs", false);
		optionBool("ice_goop", "FLUDD Weapons' black goop is ice", true);
		optionBool("gingerbread", "Gingerbread men walk about where a level puts them", true);
		optionBool("gingerbread_chase", "Gingerbread men chase and hit Mario (jump on them)", true);
		optionBool("baubles", "Bob-ombs a level names Bauble look like Christmas baubles", true);
		// FLUDD Weapons loading (now or later): told again
		on<SmsEvMod>(SMS_EV_MOD_LOADED, [this](SmsEvMod& e) {
			if (e.id && !strcmp(e.id, "fludd-weapons"))
				iceKnown = false;
		});

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
		// api->parts: right after `models` in Chase's port with it (the snowman-enemy branch), after
		// reflections and grass in Remastered: found by its magic in the pointers after `models`
		if (SMS_API_HAS(&api(), models)) {
			const size_t at = offsetof(SmsApi, models) + sizeof(void*);
			for (int k = 0; k < 3 && !parts; k++) {
				const size_t off = at + k * sizeof(void*);
				if (api().size < off + sizeof(void*))
					break;
				const SmsPartsApi* t = *(const SmsPartsApi* const*)((const char*)&api() + off);
				if (t && t->magic == SMS_PARTS_MAGIC)
					parts = t;
			}
		}
		if (!parts || !SMS_API_HAS(&api(), models) || !SMS_API_HAS(&api(), actor)) {
			warn("this game has no api->parts: no hats, clothes or snowmen");
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
		sMouth   = make(marioMouth);
		ball     = make(snowball);
		eHat     = make(elfHatMario);
		eEars    = make(elfEars);
		fluddDeco = make(fluddRibbon);
		hat     = make(santaHat);
		head    = make(snowmanHead);
		gBody   = make(gingerBody);
		{
			Mesh a0, a1, l0, l1;
			gingerArm(a0, -1), gingerArm(a1, 1), gingerLeg(l0, -1), gingerLeg(l1, 1);
			gArm[0] = api().models->create(handle(), &a0.corners[0], (int)a0.corners.size());
			gArm[1] = api().models->create(handle(), &a1.corners[0], (int)a1.corners.size());
			gLeg[0] = api().models->create(handle(), &l0.corners[0], (int)l0.corners.size());
			gLeg[1] = api().models->create(handle(), &l1.corners[0], (int)l1.corners.size());
		}
		{
			Mesh cap;
			baubleCap(cap);
			baubleCapMesh = api().models->create(handle(), &cap.corners[0], (int)cap.corners.size());
			for (int k = 0; k < 5; k++)
				for (int c = 0; c < 4; c++) {
					static const Rgb kA[4] = {kBaubleGold, kSilver, {255, 255, 255}, {200, 30, 40}};
					static const Rgb kB[4] = {{255, 255, 255}, kBaubleGold, kSilver, kBaubleGold};
					Mesh pm;
					baublePattern(pm, k, kA[c], kB[c]);
					baublePat[k][c] = api().models->create(handle(), &pm.corners[0], (int)pm.corners.size());
				}
		}
		buttons = make(snowmanButtons);
		arms    = make(snowmanArms);

		on(SMS_EV_STAGE_LOADED, [this](SmsEvent&) { dressed.clear(), snowy.clear(), bare.clear(), wasUnder.clear(), baubled.clear(), baubles.clear(), baubleLooks = 0, gingers.clear(), gingerLooks = 0; }); // the tints (and what was hidden) went with the old stage
		on(SMS_EV_TICK, [this](SmsEvent&) { tick(); });
		on<SmsEvWaterEmit>(SMS_EV_WATER_EMIT, [this](SmsEvWaterEmit& e) -> int { return onWaterEmit(e); });
		on<SmsEvWaterLand>(SMS_EV_WATER_LAND, [this](SmsEvWaterLand& e) -> int { return onWaterLand(e); });
		info("loaded: Santa hats, winter clothes, snowmen, Rudolph, Santa Mario and Christmas FLUDD");
		return 0;
	}
};

} // namespace

SMS_MOD_CLASS(ChristmasAdditions)
