# -*- coding: utf-8 -*-
import numpy as np

# basics for simulated die rolling

def roll_die(sides):
    return np.random.default_rng().integers(low = 1, high = sides + 1)

# roll_die(2)

def roll_d6_to_hit(to_hit):
    return (roll_die(6) <= to_hit)

# roll_d6_to_hit(2)

def rolls_from_n_ds(num_die_sides, num_rolls):
    "return an array with num_rolls results from rolling a die with sides from 1..num_die_sides"
    return np.random.default_rng().integers(low = 1, high = num_die_sides + 1, size = num_rolls)

import functools
rolls_from_n_d6 = functools.partial(rolls_from_n_ds, 6)

def num_hits_from_rolls_to_hit(rolls, to_hit):
    return sum(x <= to_hit for x in rolls)

