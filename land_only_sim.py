# -*- coding: utf-8 -*-
# Now make a class that works for land battles - infantry, artillery, tanks

# import numpy as np

import dice_basics

class LandForce:
    "still building up to real rules - just represent vanilla land units (no AA)"
    
    def __init__(self, i = 0, a = 0, t = 0):
        self.unit_counts = {'i': i, 'a': a, 't': t}
    
    def num_units(self):
        return(sum(self.unit_counts.values()))
    
    def rolls_from_attack(self):
        "return all attack rolls (recall infantry matched with artillery attack at 2)"
        
        num_boosted_inf = min(self.unit_counts['i'], self.unit_counts['a'])
        num_plain_inf = self.unit_counts['i'] - num_boosted_inf
        
        rolls = {1: dice_basics.rolls_from_n_d6(num_plain_inf), 
                 2: dice_basics.rolls_from_n_d6(num_boosted_inf + self.unit_counts['a']), 
                 3: dice_basics.rolls_from_n_d6(self.unit_counts['t'])}
        
        return rolls

    def rolls_from_defense(self):
        "return all defense rolls"
        
        rolls = {2: dice_basics.rolls_from_n_d6(self.unit_counts['i'] + self.unit_counts['a']), 
                 3: dice_basics.rolls_from_n_d6(self.unit_counts['t'])}
        
        return rolls
   
    def remove_losses(self, num_hits):
        "mutate object to remove hit units in IPC order"
        
        # encapsulating units counts also a good refactor...or at least making reducing a type of unit a private method
        # but will be throwing this away in next pass anyway...
        
        if num_hits <= self.unit_counts['i']:
            num_hits, self.unit_counts['i'] = 0, self.unit_counts['i'] - num_hits
        else:
            num_hits, self.unit_counts['i'] = num_hits - self.unit_counts['i'], 0 
        
        if num_hits <= self.unit_counts['a']:
            num_hits, self.unit_counts['a'] = 0, self.unit_counts['a'] - num_hits
        else:
            num_hits, self.unit_counts['a'] = num_hits - self.unit_counts['a'], 0 
        
        if num_hits <= self.unit_counts['t']:
            num_hits, self.unit_counts['t'] = 0, self.unit_counts['t'] - num_hits
        else:
            num_hits, self.unit_counts['t'] = num_hits - self.unit_counts['t'], 0 


# encapsulating this use of a dictionary would be an obvious refactor...
def num_hits_from_rolls(rolls):
    hits_by_to_hit = [dice_basics.num_hits_from_rolls_to_hit(r, x) for x, r in rolls.items()]
    return sum(hits_by_to_hit)


def land_battle(att_force, def_force):
    while (att_force.num_units() > 0 and def_force.num_units() > 0):
        num_att_hits = num_hits_from_rolls(att_force.rolls_from_attack())
        num_def_hits = num_hits_from_rolls(def_force.rolls_from_defense())
        
        att_force.remove_losses(num_def_hits)
        def_force.remove_losses(num_att_hits)
        
        # print (att_force.unit_counts, def_force.unit_counts)
        
    return(str(att_force.unit_counts), str(def_force.unit_counts))

  

from collections import Counter

print("\nSimulated 2i vs 1i:")
print(Counter([land_battle(LandForce(2, 0, 0), LandForce(1, 0, 0)) for i in range(100000)]))

print("\nSimulated 5i+5t vs 10t:")
print(Counter([land_battle(LandForce(5, 0, 5), LandForce(10, 0, 0)) for i in range(100000)]))
