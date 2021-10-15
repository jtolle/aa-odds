# -*- coding: utf-8 -*-
"""
very simple case - calc and simulate 2 infantry attacking 1 infantry, then compare
"""

import os
os.chdir("C:\\Users\\j-tol\\OneDrive\\Documents\\aa")

# pick a very simple but not completely trivial case to mess with first...
# 2 infantry attacking 1 infantry
# recall attacking infantry hit on a 1, defending on a 1 or 2

import numpy as np
from scipy.stats import binom

###
# calculated odds first...
# represent state of board with a matrix representing P(# attackers left, # defenders left)
# represent result of rolling from a given state with a matrix representing P(# attacker hits, # defender hits)

def joint_pmf_sums_near_one(joint_pmf):
    return(abs(sum(sum(joint_pmf)) - 1) < 0.000001)

# take a matrix representing joint pmf # of attacker and defender hits
# return joint pmf without "nobody got a hit" case
def eliminate_zero_hits_from_joint_pmf(joint_pmf):
    p_zero_hits = joint_pmf[0, 0]
    
    # distribute the [0, 0] case as the sum of a geometric series
    new_pmf = joint_pmf
    new_pmf[0, 0] = 0
    new_pmf = new_pmf * (1 / (1 - p_zero_hits))
    
    assert joint_pmf_sums_near_one(new_pmf)
    
    return(new_pmf)

# explicitly step through mini-example of more general algorithm for this:
# 1) start the state of the board at (# att, # def) with probability 1
# 2) for each "non-stopping" state remaining, distribute its probability to other states
# 3) repeat #2 until there are no more non-stopping states

# Think about a different algorithm when making a calculator for more general cases.
# Using a Markov chain is conceptually simpler once "state" can't be expressed easily by
# just # of attackers and # of defenders remaining. In 1995 that was out of the question
# due to the size of the state space for interesting battles, at least on my computer then!

def distribute_non_stopping_state(state_pmf, n_att, n_def, joint_hits_pmf):
    
    # in going this direction, assert this state isn't downstream of another state with P>0
    # probably also compute the hits pmf based on the state - TBD
    
    assert joint_hits_pmf[0, 0] == 0
    
    new_state = np.ndarray.copy(state_pmf)
    
    p_in_state = state_pmf[n_att, n_def]
    
    # should encapsulate whatever this becomes if using this algorithm
    # until all "distributing" is done, the state pmf is invalid
    new_state[n_att, n_def] = 0
    for (a_hits, d_hits), joint_hits_p in np.ndenumerate(joint_hits_pmf):
        new_n_a = max(0, n_att - d_hits)
        new_n_d = max(0, n_def - a_hits)
        new_state[new_n_a, new_n_d] += (p_in_state * joint_hits_p)
    
    assert joint_pmf_sums_near_one(new_state)
        
    return(new_state)

# make sure to send floats not ints!    
battle_2i_v_i1 = np.array([[0, 0], [0, 0], [0, 1.0]])

# 2i attacking 1i
joint_hits_pmf_2i_v_1i = eliminate_zero_hits_from_joint_pmf(np.outer(binom.pmf([0, 1, 2], 2, 1/6), binom.pmf([0, 1], 1, 1/3)))
assert joint_pmf_sums_near_one(joint_hits_pmf_2i_v_1i)

battle_if_1_1_is_stopping_state = distribute_non_stopping_state(battle_2i_v_i1, 2, 1, joint_hits_pmf_2i_v_1i)

# 1i attacking 1i
joint_hits_pmf_1i_v_1i = eliminate_zero_hits_from_joint_pmf(np.outer(binom.pmf([0, 1], 1, 1/6), binom.pmf([0, 1], 1, 1/3)))
assert joint_pmf_sums_near_one(joint_hits_pmf_1i_v_1i)

battle_if_one_side_must_win = distribute_non_stopping_state(battle_if_1_1_is_stopping_state, 1, 1, joint_hits_pmf_1i_v_1i)

print("\nCalc'd 2i vs 1i:")
print(battle_if_one_side_must_win)
###


###
# simulated odds next...
# absolute simplest simulator - just infantry

import dice_basics

def sim_infantry_battle(n_att, n_def):
    while n_att > 0 and n_def > 0:
        att_hits = sum([dice_basics.roll_d6_to_hit(1) for i in range(n_att)])
        def_hits = sum([dice_basics.roll_d6_to_hit(2) for i in range(n_def)])
        
        n_att = max(0, n_att - def_hits)
        n_def = max(0, n_def - att_hits)
        
        # print("Hits:", "att", att_hits, "def", def_hits, "; Remain:", "att", n_att, "def", n_def)
        
    return n_att, n_def
        
from collections import Counter

print("\nSimulated 2i vs 1i:")
print(Counter([sim_infantry_battle(2,1) for i in range(100000)]))
# Counter([sim_infantry_battle(3,2) for i in range(100000)])  


