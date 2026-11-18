# Player guide and screenshots

[Play and project overview](../../README.md) · [Development guide](../development/README.md)

A closer look at the Backyard campaign, what to bring into a level, and how the
plants and zombies work. The screenshots and gameplay GIFs below come from the
current ROM running in mGBA. Late-game captures use the debug ROM to open those
levels. Images are enlarged without smoothing the pixels.

## Starting a game

Choose **Play** on the title screen, then pick a save slot. Empty slots offer
**New Game**; existing slots offer **Continue** and **Delete**. Each slot has its
own campaign progress and zombie kill counts. Deleting a slot opens a confirmation
screen with Cancel selected.

![The save slots, progress summaries, and delete confirmation](../media/screenshots/save_slots.png)

Continue opens the level screen. Clear a level to unlock the next one. You can
revisit any cleared level, but its lawn size and available plants stay the same
as on your first visit.

## A planted lawn

These scenes show a garden built during Backyard 1-3, with Sunflowers at the
house end, Peashooters behind the front line, and Wall-nuts farther out.

![An established garden across five lanes](../media/screenshots/developed_backyard_late.png)

![The garden during play](../media/gameplay/developed_backyard.gif)

## Learning the lawn

**Backyard 1-1** gives you one lane and Peashooter. Follow the flashing gold cell
to plant, then collect falling sun with B. The zombies wait until you've finished
the lesson.

![The first Peashooter lesson](../media/screenshots/tutorial_peashooter.png)

**Backyard 1-2** opens three lanes and adds Sunflower. First, remove the three
wilted plants with Select + A. Then plant a Sunflower in the marked cell beside
the house and collect the sun it makes. The first lesson sun arrives quickly;
normal production follows afterward.

![Clearing the wilted plants with the shovel](../media/gameplay/wilted_tutorial.gif)

**Backyard 1-3 onward** uses all five lanes. You can plant in nine cells along each
lane. Fossils decorate the bare dirt around the smaller lawns, including a T. rex.

| First level: one lane | Second level: three lanes |
| --- | --- |
| ![One-lane lawn](../media/screenshots/lawn_start.png) | ![Three-lane lawn](../media/screenshots/lawn_three_lanes.png) |

## What each clear unlocks

| Level | Lawn | What's new | Plant earned |
| --- | --- | --- | --- |
| 1-1 | One lane | Planting, sun collection, and your first hordes | Sunflower |
| 1-2 | Three lanes | Shoveling and growing your own sun | Wall-nut |
| 1-3 | Five lanes | Coneheads | Potato Mine |
| 1-4 | Five lanes | Bucketheads | Snow Pea |
| 1-5 | Five lanes | Newspaper Zombies | Cherry Bomb |
| 1-6 | Five lanes | Screen Doors and tombstones | Squash |
| 1-7 | Five lanes | Football Zombies | Tall-nut |
| 1-8 | Five lanes | Pole Vaulters | Repeater |
| 1-9 | Five lanes | A mix of fast attackers and shields | Chomper |
| 1-10 | Five lanes | Every zombie type in the final yard | Trophy |

The enemy mix changes between levels. A tougher zombie appearing doesn't mean
it will be in every later yard. The new plant is awarded before the level where
its matching enemy first appears: for example, you earn Tall-nut before meeting
Pole Vaulters.

![Progress on the level screen](../media/screenshots/levels_progress.png)

The current game ends at Backyard 1-10. Night and Pool aren't playable worlds yet.

## Scouting and seed selection

The camera travels from the lawn to the road before each round. The road shows
the zombie types scheduled for that level, with a number beside each type. Those
counts cover the incoming road zombies; living tombstones can raise more during
play.

![The camera moving from the yard to the incoming zombies](../media/gameplay/scouting.gif)

Press A/START to continue. If the level offers six plants or fewer, you take all
of them. With a larger roster, a seed grid opens over the road. Choose exactly
six packets: A adds or removes a plant, and START confirms your bank. The packets
move between their cells and the tray at the top. You can still see the incoming
zombies beside the selection panel.

![Packets moving into the six-slot tray](../media/gameplay/seed_bank_flight.gif)

An incomplete bank stays on the selection screen and shows **SELECT 6 SEEDS**.
Press B to go back to scouting. You can take your time here; the level hasn't
started yet.

[See a custom bank](../media/screenshots/seed_selection_custom.png) · [See the incomplete-bank message](../media/screenshots/seed_selection_error.png)

## Sun, packets, and placement

You begin a round with **175 sun** and about **15 seconds** before the first
zombies, after any unfinished tutorial. Falling sun and Sunflowers each provide
25 sun per pickup. Sky sun arrives about every ten seconds; a Sunflower normally
produces about every nine seconds. Sun disappears after six seconds if you leave
it on the lawn.

B collects available sun across the lawn. Holding B keeps collecting new drops.
The sun flies to the counter as you collect it, so you can keep moving the cursor
and planting.

![Sun collection from pickup to the counter](../media/screenshots/sun_collection.png)

Each seed packet shows an icon and its sun price. Gray means you can't plant it
yet; color fills upward as it recharges. L/R cycles through the bank, and holding
either button keeps cycling.

A successful planting shows a check. An occupied cell shows a cross. If the
packet is still recharging, an hourglass appears at the planting cell. A rejected
purchase also pulses the packet's border.

![Successful planting, an occupied cell, recharge, and an unaffordable packet](../media/screenshots/placement.png)

Hold Select and press A to remove a plant. There is no sun refund. The shovel
can't remove tombstones.

## Picking plants for the job

Recharge is the wait before you can place another of the same plant. It is
separate from a plant's own attack, arming, or chewing time.

| Plant | Sun | Recharge | How to use it |
| --- | ---: | ---: | --- |
| Peashooter | 100 | 3 s | Put steady fire behind your defenses. It can shoot tombstones before enemies arrive. |
| Sunflower | 50 | 4 s | Plant near the house to build your sun supply. Produces 25 sun roughly every 9 seconds. |
| Wall-nut | 50 | 10 s | Stall a lane while shooters work. Its face and shell show how much punishment it's taken. |
| Potato Mine | 25 | 15 s | Plant ahead of an approaching zombie. It takes 8 seconds to arm. |
| Snow Pea | 175 | 7.5 s | Slow walking and biting so the rest of the lane has more time to attack. |
| Cherry Bomb | 150 | 20 s | Use its short-fuse blast against a cluster in up to three neighboring lanes. |
| Squash | 50 | 30 s | Place near trouble. It leaps onto a nearby target and crushes through armor. |
| Tall-nut | 125 | 15 s | Has twice Wall-nut's health and prevents a Pole Vaulter's jump. |
| Repeater | 200 | 3 s | Fires two peas in quick succession. Useful where a single Peashooter can't keep up. |
| Chomper | 150 | 7.5 s | Swallows a nearby grounded zombie through armor, then chews for 15 seconds. Protect it while it eats. |

### Mines, bursts, and bites

A Potato Mine's light tells you when it's ready. An unarmed mine can be eaten,
so give it space to finish arming. Once triggered, it explodes in its own lane.

![A Potato Mine arming, waiting, exploding, and leaving an empty cell](../media/screenshots/mine.png)

Repeater fires twice per attack, with a separate recoil for each pea. Squash
waits for something close enough, then leaps and lands on it.

| Repeater | Squash |
| --- | --- |
| ![Repeater's two-shot burst](../media/gameplay/repeater_burst.gif) | ![Squash leaping onto a target](../media/gameplay/squash_leap.gif) |

Chomper waits with its jaws open, pulls a zombie into its mouth, and chews with
bits of its meal sticking out. It can eat a zombie that's biting a Wall-nut in the
next cell, so a nut in front doesn't put that target out of reach.

![Chomper biting and pulling in a zombie](../media/gameplay/chomper_bite.gif)

[Browse every plant's Almanac entry](../media/screenshots/almanac_plant_entries.png)

## Know the zombies

| Zombie | What to watch for | What helps |
| --- | --- | --- |
| Normal | A steady walker that will eat through an unguarded lane. | Peashooters and a little preparation. |
| Conehead | Extra protection before you're down to the ordinary zombie. | More shooting time, a nut, or an armed Mine. |
| Buckethead | A bucket that takes far more peas to break. | Concentrated fire, slowing, or a plant that bypasses armor. |
| Newspaper | The paper absorbs damage. Once it breaks, the zombie runs. | Snow Pea to slow the rush, or a blast before it reaches you. |
| Screen Door | The door blocks peas and their freezing effect while intact. | Cherry Bomb, Squash, or Chomper. |
| Flag | Leads a horde and speeds up nearby zombies' walking and biting. | Taking down the carrier makes its neighbors easier to handle. |
| Football | A fast charge and a very tough helmet. | Have a Mine ready, slow it down, or use Squash or Chomper. |
| Pole Vaulter | Jumps the first ordinary plant it reaches, then slows down. | Tall-nut stops the jump; otherwise keep another defense behind the first. |

![All eight zombie types in the Almanac](../media/screenshots/almanac_zombie_entries.png)

Snow Pea slows both movement and biting. A frosted zombie has a blue tint; a hit
flashes red. Plants also flash red when damaged.

![Snow Pea fighting a slowed zombie](../media/screenshots/snow_pea_gameplay.png)

As zombies take damage, equipment breaks away and arms fall off. Defeated zombies
collapse onto the lawn. Flag Zombies drop their flags and fall backward.

![Damage, a falling arm, and a zombie collapsing during gameplay](../media/gameplay/zombie_damage_death.gif)

## Zombie damage and death

Each GIF below shows equipment and arm loss, wounded movement, and death with
and without a missing arm. These close-ups are assembled from the current sprite
sheets, with the stages shown together for comparison. The top row shows damage
and wounded movement; the bottom row shows deaths with both arms and after arm
loss. They are animation studies, not recordings of combat.

| Normal | Conehead |
| --- | --- |
| ![Normal zombie damage, arm loss, and both death variants](../media/studies/zombie_normal_damage_death_study.gif) | ![Conehead losing its cone and arm, then dying](../media/studies/zombie_conehead_damage_death_study.gif) |

| Buckethead | Newspaper |
| --- | --- |
| ![Buckethead armor damage, bucket drop, arm loss, and death](../media/studies/zombie_buckethead_damage_death_study.gif) | ![Newspaper breaking, arm loss, running wounded, and death](../media/studies/zombie_newspaper_damage_death_study.gif) |

| Screen Door | Flag |
| --- | --- |
| ![Screen Door damage, door drop, arm loss, and death](../media/studies/zombie_screen_door_damage_death_study.gif) | ![Flag Zombie arm loss, flag drop, and backward fall](../media/studies/zombie_flag_damage_death_study.gif) |

| Football | Pole Vaulter |
| --- | --- |
| ![Football helmet damage, helmet and arm drops, and death](../media/studies/zombie_football_damage_death_study.gif) | ![Pole Vaulter vault, pole and arm drops, and death](../media/studies/zombie_pole_vaulter_damage_death_study.gif) |

[Equipment close-ups](../media/studies/falling_equipment_study.gif) · [Flag death in gameplay](../media/gameplay/flag_death.gif) ·
[Plant damage flash](../media/screenshots/plant_red_flash.png)

## Hordes and tombstones

Follow the progress bar across the top. Each flag marks an incoming horde, and
turns yellow when its Flag Zombie enters the screen. Ordinary groups arrive
between flags. Once half a group has been defeated, the next can begin arriving;
you don't get a fresh preparation break between them. The final horde is larger
than the earlier ones.

![Five horde flags and the moving progress indicator](../media/screenshots/wave_track_5.png)

Tombstones appear from 1-6. You can't plant on them, and peas hit the stone before
they can reach a zombie behind it. Six peas break one; cracks show its damage.
Bombs and mowers can destroy them too.

Each standing tombstone raises a zombie when a horde leader arrives. The zombie
climbs out of the dirt before walking or biting, and you can hit it during the
climb. Destroying the stone prevents later raises, but won't cancel one already
on its way out.

| Breaking a tombstone | A zombie emerging |
| --- | --- |
| ![A tombstone crumbling](../media/gameplay/tombstone_crumble.gif) | ![A zombie crawling out of the dirt](../media/gameplay/grave_emergence.gif) |

## Mowers, wins, and defeats

Each active lane has one mower beside the house. If a zombie gets that far, the
mower races along the lane and clears its path. It's gone for the rest of the
round, so rebuild that defense before another zombie breaks through.

![A mower clearing its lane](../media/screenshots/mower_active.png)

After the last zombie, any moving mower or active explosion finishes before the
reward arrives. A first clear with a new unlock earns a bouncing seed packet.
Repeat clears and rounds without a plant reward earn a trophy. The lawn and HUD
stay visible behind the reward and its wooden sign.

![A Sunflower packet bouncing onto the lawn](../media/gameplay/plant_reward.gif)

After the reveal, any button returns to levels. Clearing 1-10 also returns there,
so you can replay the campaign.

![A trophy for a repeat clear](../media/screenshots/trophy_reward.png)

If a zombie reaches the house after the mower is gone, the music stops, followed
by a record scratch and the defeat scream. Wait for the sequence to finish, then
press A/START to retry through scouting or B to choose another level.

![The defeat sequence and retry prompt](../media/gameplay/defeat.gif)

## Almanac and audio

Choose **Almanac** from the title screen. Plants and zombies appear in cells; open
one with A to read its description and stats. Portraits animate while you read.
Plant entries show sun cost, recharge, and health. Zombie entries combine body
health and protection into durability and show your saved kills of that type.
Kill counts belong to a save slot: continue that slot before opening the Almanac
to check its records.

L/R changes pages, D-pad moves within the current grid, and B goes back. On an
entry, Up/Down scrolls the description; the bar on the right shows your position.

| Plant grid | Zombie entry and saved kills |
| --- | --- |
| ![The plant grid](../media/screenshots/almanac_plants.png) | ![A zombie entry with lifetime kills](../media/screenshots/almanac_lifetime_kills.png) |

START pauses a round. The menu offers Resume, Restart, and Levels, plus separate
Master, Music, and Effects sliders. Use Up/Down to choose a slider and Left/Right
to adjust it. The settings last through restarts and menu visits, then reset when
you relaunch the ROM.

![Pause menu with the three volume sliders](../media/screenshots/paused.png)

## Controls at a glance

| Screen | Controls |
| --- | --- |
| Lawn | D-pad: cursor · A: plant · L/R: seed · B: collect sun · Select + A: shovel · START: pause |
| Title | Up/Down: select · A/START: open |
| Save slots | Up/Down: slot · Left/Right: action · A: confirm · B: title |
| Delete confirmation | Left/Right: Cancel/Delete · A: confirm · B: cancel |
| Levels | D-pad: select · A/START: scout · B: title |
| Scouting | A/START: continue · B: levels |
| Seed selection | D-pad: select · A: add/remove · START: confirm six · B: scouting |
| Almanac grid | D-pad: select · L/R: page · A: details · B: title |
| Almanac details | Up/Down: scroll · B: grid |
| Pause | Up/Down: select · Left/Right: volume · A: action · B/START: resume |
| Reward | Any button: levels, after the reveal |
| Defeat | A/START: retry · B: levels, after the sequence |

Holding the D-pad repeats cursor movement. Holding L/R cycles seeds. Holding B
collects sun as it appears.

## More images

[All plant entries](../media/screenshots/almanac_plant_entries.png) · [All zombie entries](../media/screenshots/almanac_zombie_entries.png) ·
[Wall-nut damage](../media/screenshots/wallnut.png) · [Lawn edge and mower parking](../media/screenshots/lawn_edge.png) ·
[The final skull flag](../media/screenshots/final_flag.png) · [Full gameplay animation](../media/gameplay/gameplay.gif)

The following close-up studies are assembled from sprite sheets rather than
captured gameplay: [Football charge](../media/studies/football_charge_study.gif),
[Pole Vaulter poses](../media/studies/pole_vaulter_study.gif), [Screen Door biting](../media/studies/screen_door_bite_study.gif),
[falling equipment](../media/studies/falling_equipment_study.gif), and [zombie deaths](../media/studies/zombie_deaths_study.gif), and [Pole Vaulter deaths](../media/studies/vault_deaths_study.gif).

In the Pole Vaulter study, the upper row is intact and the lower row is wounded.
Columns show walking with the pole, biting with the pole, biting after the jump,
and vaulting. The Screen Door study shows intact and damaged doors side by side.
The equipment study reads left to right: arm, newspaper arm, cone, newspaper,
bucket, door, flag, football arm, helmet, pole.

For build instructions, tests, audio credits, and screenshot capture tools, see the
[development guide](../development/README.md).
