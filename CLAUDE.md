# Notes for Claude working on Christmas Additions

- Allebale's mod (house rules: see the fork's CLAUDE.md in Allebale/sms-pc-port-remastered: commit author,
  settings text, no variables named far or near, ask before releases).
- Needs Super Mario Sunshine Remastered's api->parts (the SDK fork's `parts` branch, the game's
  `snowman-enemy` branch). Everything Christmas lives here, never in the game (Chase's rule).

## Being made, one at a time (Allebale, Nov 19: "start now, one at a time")

Christmas Mountain got a Yoshi egg by the start, with a coconut, papaya, pineapple and banana beside it
(the game picks one of those outside Delfino Plaza; the egg only shows once Yoshi is freed in the Plaza).
Next, in order:

- **Yoshi as Rudolph**: done (1.2.4, Allebale: "looks great now"). Snow on his nose an option, Off. Built (1.2.1): brown via parts->yoshi_color; antlers, ears and a red nose made in
  yoshi_model.bmd's rest pose (Allebale sent yoshi.szs: jnt_head at 0 101 0, its x forward, y down, z left;
  null_nose 0 144 56; snout tip z 63; head top 168) and drawn on jnt_head. Materials: _mat_yo_body, _head,
  _nose (the nostrils), _mouse, _eye_L/R, _foot1_R (feet; (3) is the saddle), _hand1_R.
- **Santa Mario** and **Christmas FLUDD** (1.3.0, first try; Allebale sent mario.szs): tints by material
  (ma_mdl1: _mat_head shirt and trousers, (2) the bib, (3)/(4) shoes, (5)/(6) gloves, (7) cap, (8) face; one
  atlas, and a tint name matches every name holding it, so all "_mat_head" then "_mat_head(" back), a Santa hat,
  belt, sleeve and trouser fur on jnt_head, jnt_waist, jnt_arm_R2/L2, jnt_leg_R2/L2 (rest frames in the
  source), a beard option (Off). FLUDD (wg_mdl1, on jnt_G_main): _mat_G_main red, froater(2) (the pump) green,
  froater(3) (handles) gold; the tank (_mat_G_main_tank, a drum along x -3..33 round y -40) wrapped in a gold
  ribbon with a bow and holly on its back (-y). The nozzles are models of their own, untouched. 1.3.1: his cap
  off under the Santa hat (player SMS_PLAYER_PART_CAP and parts->hide of _mat_head(7), game 251a788), and a
  no-moustache option. 1.4.0: the moustache really gone with the beard: its 31 vertices (a piece of its own
  in the face) folded to a point inside his head with parts->set_vertex (game 9877918), put back after; a
  skin-coloured cover only left a flesh moustache (Allebale).
- **FLUDD throws snowballs** (1.5.0, first try, option Off): SMS_EV_WATER_EMIT of the spray nozzle cancelled, a
  drop of the mod's own (water->emit, tag "SN", unseen) every 0.16 s, 1.9 times as fast, bigger, drawn as a
  snowball mesh; SMS_EV_WATER_LAND bursts it into snow puffs. Hits as water does, uses the tank (6 drops).
- **Piantas under the Santa hat** (1.7.0): their own hat (worn models "hat..." and "hw...") left off with
  api->parts' hide_worn, and the palm-leaf hair hidden with hide_joint ("yashi_jnt", "leafL_jnt",
  "leafR_jnt"; game 3818444, SDK 80f47b0). Allebale sent bianco0.szs: montemc/momc_model.bmd and
  montew/mow_model.bmd (materials _eye_mat, _fuku_mat (men), _hand_mat, _leafback_mat).

## Later (Allebale, Nov 20: "add later")

- **Ice weapon**: when Christmas Additions is installed, the FLUDD Weapons mod's black goop weapon becomes
  an ice weapon, its textures white-blueish ice. Must stay a mod change (Chase's rule); the two mods are
  separate, so Christmas Additions has to find and retexture the goop weapon's look itself, or FLUDD
  Weapons offers a general hook for it. Ask whether it should also behave like ice (freeze, slippery)
  or only look icy.
