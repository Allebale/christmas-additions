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
  no-moustache option (skin-coloured copy of the moustache triangles, with the beard).
- **FLUDD shoots fast snowballs** in place of water, as a mod.
