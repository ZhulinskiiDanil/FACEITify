# FACEITify

Replaces the difficulty faces with FACEIT skill level badges.

Every face in the game turns into a badge: level cells, the level page, search
results, profiles, leaderboards, and the difficulty filters in the search menu.

## How difficulties map

Easy through Insane become levels **1 to 5**, Easy Demon through Extreme Demon
become **6 to 10**, and a plain Demon counts as a hard one. Levels GD hasn't
rated show a **?**.

The ring around the badge fills up with the level, the way it does on a real
FACEIT profile, and the number takes the colour of its tier.

Auto levels have no skill to measure, so instead of a number they get a badge of
their own, with the ring sitting at the far end in blue.

## Challenger

Levels in the **top 100 of the classic demon list** get the Challenger badge
instead, with their place on the list inside it.

The list also settles the demons GD never got round to rating: if a level is on
it at all, it counts as a 10 rather than a **?**.

Placements come from [demonlist.org](https://demonlist.org). The whole list is
fetched once and kept for an hour, so browsing levels costs no requests. With no
connection the mod falls back to the difficulties GD itself reports, and nothing
breaks.

## Credits

Thanks to **xboctatuk** and [Global List
Integration](https://github.com/XBOCTATUK/Global-List-Integration) for working
out the demon list API first.
