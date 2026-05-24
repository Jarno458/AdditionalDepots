This mod provides cheat commands to alter the contents of different depots created by  Additional Depots mod. This mod is intended for developers that use the Additional Depots mod. its not intended to be used by end users

This mod adds the following / commands

All commands need the depot identifier that is set in depot definition. <br>
This mod also adds 2 Test depots, `G` for a global test depot shared between all players and `P` for a Player specific test depot. <br>
If altering an player specific depot, but nu player is specific, it will use the current player's its depot

/ad-set `depot-identifier` `amount` `"item-name"` (optional: `player name`) <br>
Sets an item to a specific amount for a specific depot 

/ad-add `depot-identifier` `amount` `"item-name"` (optional: `player name`) <br>
Adds an item to a specific depot (can add negative to subtract)

/ad-addauto `depot-identifier` `amount` `"item-name"` `interval` <br>
Adds an amount of items to the depot every interval seconds, ideal for testing how code reacts to changes of the amount over time (can add negative to subtract)

/ad-addmany `depot-identifier` `amount` `"item-name 1"` `"item-name n"` (optional: `player name`) <br>
Adds many items to a specific depot with the same amount (can add negative to subtract)

/ad-setcontent `depot-identifier` `content preset index (0-3)` (optional: `player name`) <br>
Overrides the content of a specific depot with a set preset
Presets:
 * 0: Empty
 * 1: 500 Iron Ingot, 1000 Iron Plate, 50 Iron Rod, 25 Screws
 * 2: 5 Silica, 20 Concrete, 5 Quartz Crystals
 * 3: 10 Modular Frame, 5 Heavy Modular Frame, 20 Fused Modular Frame

/ad-config `depot-identifier` `<CanDragItemsToInventory true/false` `CanBeUsedWhenBuilding true/false` <br>
Updates the runtime configuration of a specific depot and specifies if items can be dragged out of it and if it can be used automatically when building

/ad-max `depot-identifier` `max-type` `amount` <br>
Updates the max amount and max amount type of a specific depot
Max Type can be one of the following:
 * Stacks: amount is the amount of stacks it can store
 * Total: amount is an numeric hard cap, entries will show (current/max) and a progress bar of how filled the entry is
 * TotalHidden: amount is an numeric hard cap, entries will show current amount and a progress bar of how filled the entry is
 * None: the numeric hard cap is the INT32_MAX, entries will show current amount and no progress bar will be shown

/ad-ps <br>
Prints out the current connected players to debug their states