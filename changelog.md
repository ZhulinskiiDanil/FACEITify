# Changelog

## v1.2.0

- Demons on the list below the top 100 are graded from **11** to **20** by where
  they sit in it, instead of all counting as a **10**. The bands follow the list:
  they are hundreds of places wide down the tail and a dozen wide near the
  Challenger cut, because that is where the difficulty actually jumps.
- The ring is full from **10** up, so the new levels read off the number and
  FACEIT's own colour for it: red into magenta, then the blues, then white at
  **20**. Challenger's ring is gold.
- **Pointercrate** is read to the end of the legacy list rather than the first
  two hundred places, so the lower grades have something to sit on.

## v1.1.0

- RobTop's own levels get badges too. The main level select draws its face by
  hand rather than through the class every other face goes through, so it kept
  the vanilla one ([#1](https://github.com/ZhulinskiiDanil/FACEITify/issues/1)).
- The badge ring fills up to the level when a level page opens. A Challenger
  that the list confirms late fills it again. There is a setting for people who
  would rather it sat still.
- Placements can come from **Pointercrate** or **AREDL** instead of
  demonlist.org, and **Off** stops the mod asking anyone. Each list keeps its
  own cache.
- **More Difficulties** is marked incompatible. Both mods want the same faces,
  and its tiers have no skill level to turn into.

## v1.0.1

- The difficulty filters ring whichever toggle is on: the badges took GD's own
  highlight with them. The demon type button follows the Demon toggle, and the
  demon type picker rings the type you last chose.
- Auto levels get a badge of their own instead of a number, with the ring
  sitting at the far end in blue. Levels GD hasn't rated keep their question
  mark.
- The Auto filter kept the vanilla face where everything else was a badge. It
  gets one too.

## v1.0.0

First release.

- Difficulty faces are replaced with FACEIT skill level badges everywhere the
  game draws them, the search filters included.
- Easy through Insane map to levels 1-5, the demons to 6-10, unrated levels to a
  question mark.
- The badge ring fills with the level and the number is coloured by tier.
- Levels in the top 100 of the classic demon list get the Challenger badge with
  their placement in it, and anything else on the list counts as a 10.
- Placements come from demonlist.org, cached for an hour. No connection means no
  Challenger badges and nothing else missing.
