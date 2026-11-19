# Notes for Claude working on Christmas Additions

- Allebale's mod (house rules: see the fork's CLAUDE.md in Allebale/sms-pc-port-remastered: commit author,
  settings text, no variables named far or near, ask before releases).
- Needs Super Mario Sunshine Remastered's api->parts (the SDK fork's `parts` branch, the game's
  `snowman-enemy` branch). Everything Christmas lives here, never in the game (Chase's rule).

## Being made, one at a time (Allebale, Nov 19: "start now, one at a time")

Christmas Mountain got a Yoshi egg by the start, with a coconut, papaya, pineapple and banana beside it
(the game picks one of those outside Delfino Plaza; the egg only shows once Yoshi is freed in the Plaza).
Next, in order:

- **Yoshi as Rudolph** (1.2.1): brown via parts->yoshi_color; antlers, ears and a red nose made in
  yoshi_model.bmd's rest pose (Allebale sent yoshi.szs: jnt_head at 0 101 0, its x forward, y down, z left;
  null_nose 0 144 56; snout tip z 63; head top 168) and drawn on jnt_head. Materials: _mat_yo_body, _head,
  _nose (the nostrils), _mouse, _eye_L/R, _foot1_R (feet; (3) is the saddle), _hand1_R.
- **A Santa costume for Mario**, as a mod.
- **FLUDD shoots fast snowballs** in place of water, as a mod.
- **Christmas FLUDD**: every FLUDD model reskinned to look Christmassy, in first person (held in view) and third
  person (on his back), as a mod.
