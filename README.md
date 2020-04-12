
# AUNTY LANGTON'S FREE
[![Donate](https://img.shields.io/badge/Donate-PayPal-green.svg)](https://www.paypal.com/cgi-bin/webscr?cmd=_donations&business=YQX9FWDCXSK34&currency_code=AUD&source=url)
## Aunty Langton's Musical Ant

### Description

This module is a semi-random sequencer based on the Langton's Ant cellular automaton. Langton's Ant demonstrates *emergent* behaviour. That is, complex or unforseen behaviour emerges within a system based upon a simple set of rules.

The Basic Rules:
- The ant exists in an X by Y, 2D plane of discrete squares which have only two possible states each, black or green.
- The ant starts facing upwards and moves forward one step, leaving the previous square green
- Depending on the colour of the square the ant lands on, the ant will either change it's direction by 90 degrees left or right.

Learn more about Langton's Ant [here](https://en.wikipedia.org/wiki/Langton%27s_ant) on Wikipedia.

In this implementation of Langton's Ant, the module outputs a quantized X and Y voltage depending on the X or Y position of the ant on the 2D plane. This quantized voltage can be changed by the Octave, Note and Pitch knobs. These two voltages are combined into a Poly output port.

There is also a Shadow Ant which, when enabled, will output an additional two X and Y voltages correlated to the position of the Shadow Ant. The Shadow ant likes to clean up after the primary ant, and sometimes they dance together in some strange, repeating period. (the voltage output of which, can sound interesting when controlling oscillators!) Eg.

![ShadowDance](https://media.giphy.com/media/SRrOaKWPQkBuJjyC7E/source.gif)

There is *also* a loop switch that allows the user to loop over up to 32 steps. Try drawing near the ant(s) while looping and you will see interesting, evolved sequences due to your interference with the environment. You can also make the ant(s) skip steps which increases the scale of jumps between voltage outputs.

### Panel Overview

![Panel Overview](https://community.vcvrack.com/uploads/default/original/2X/8/8475f6ad2e0d80d4d479f97bea509d987d3bc313.png)

## Thanks

Andrew Belt (VCV Rack creator) - Thank you for creating and open sourcing such an amazing creative tool!
For info on VCV Rack check out: https://vcvrack.com/

JW Modules - Thank you very much for open sourcing your modules Jeremy, I have learnt so much from them!
See a bunch of Jeremy's excellent work here: http://jeremywentworth.com/

DSEG LCD-style font - Thank you Keshikan けしかん!
Find this font here: https://github.com/keshikan/DSEG

Christopher Langton - Of course, thanks to the creator of the Ant!
https://en.wikipedia.org/wiki/Christopher_Langton

<br><br>
[![Donate](https://img.shields.io/badge/Donate-PayPal-green.svg)](https://www.paypal.com/cgi-bin/webscr?cmd=_donations&business=YQX9FWDCXSK34&currency_code=AUD&source=url)
