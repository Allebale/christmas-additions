# Christmas Additions

A mod for the Super Mario Sunshine PC port that brings Christmas to Isle Delfino. Every part can be
switched on or off in the game's Mods menu.

- Piantas wear Santa hats (their own hats and leaf hair come off) and winter colours
- Strollin' Stu stacks become snowmen, with top hats, coal faces and carrot noses
- Falling snow, and white footprints and snow puffs in snowy levels
- Yoshi is Rudolph the red-nosed reindeer, with antlers and a glowing nose
- Mario can dress as Santa or as an elf, with an optional beard and no moustache
- FLUDD comes wrapped for Christmas, and can throw snowballs (off by default)
- Coming back out of a level's door starts Mario beside it

## Install

Download the zip from the latest release and unzip it into the game's `mods/` folder.

## Needs

A game with `api->parts`: Super Mario Sunshine Remastered, or its `snowman-enemy` build. Without it the
mod loads but leaves the hats, clothes and snowmen out.

## Build

```
cmake -B build -S .
cmake --build build
```

The SDK it builds against is in `sdk/`.
