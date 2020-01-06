
# AUNTY LANGTON'S FREE

# Aunty Langton's Musical Ant

This module is a semi-random sequencer based on the Langton's Ant cellular automaton. Langton's Ant demonstrates *emergent* behaviour. That is, complex behaviour emerges within a system based upon a simple set of rules.

The Basic Rules:
- The ant exists in an X by Y, 2D plane of discrete squares which have only two possible states each, black or green.
- The ant starts facing upwards and moves forward one step, leaving the previous square green
- Depending on the colour of the square the ant lands on, the ant will either change it's direction by 90 degrees left or right.

In this implementation of Langton's Ant, the module outputs a quantized X and Y voltage depending on the X or Y position of the ant on the 2D plane. This quantized voltage can be changed by the Octave, Note and Pitch knobs. These two voltages are combined into a Poly output port.

There is also a Shadow Ant which, when enabled, will output an additional two X and Y voltages correlated to the position of the Shadow Ant. The Shadow ant likes to clean up after the first any and sometimes they dance together in some strange period.

There is *also* a loop switch that allows the user to loop over up to 32 steps. Try drawing near the ants why looping and you will see interesting evolved sequences.


# Thanks

For info on VCV Rack check out: https://vcvrack.com/
(Thank you Andrew Belt for creating such an amazing tool)

JW Modules - Thank you very much for open sourcing your modules Jeremy, I have learnt so much from them! http://jeremywentworth.com/

Font usage: https://github.com/keshikan/DSEG
(thanks Keshikan けしかん!)
