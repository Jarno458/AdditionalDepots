# Additional Depots
This mod allows other mod authors to add Additional sources if items to their players, in the form of depots, similar to the dimensional depot. The mod author will have full control over what items are available in the depot and how it behaves for the player.
<iframe width="560" height="315" src="https://www.youtube.com/embed/mH_7AXQ7gwM" frameborder="0" allowfullscreen></iframe>

### Creating Depot Definition
In order to implement your own depots with the Additional Depots mod, you should get the code from github and place it inside a folder named `AdditionalDepots` inside your `SatisfactoryModLoader\Mods\` folder, then re-generate the visual studio project files and build the your modding projects developer-editor cpp side. once that is done open the edit and create a new resource of the type `AdditionDepotDefinition` and fill in the details as described below. Depot definition files do not need to registered, they are picked up automatically.
<TODO INSERT IMAGE>

#### Depot Definition settings
<TODO INSERT IMAGE>

The setting work as follows:  
Identifier:  
`Identifier`: an unique FName that is used to reference your specific depot by code (not visible to the user).  
Presentation:  
`Name`: This display name of your depot.  
`Color`: The color corresponding to this depot, it wil be used to visually represent the depot in for example build costs.  
`Icon`: An Icon that is displayed in player inventory menu for your depot.  
Configuration:  
`Max Type`: Controlls how many of a single item can be stored in the depot (can be changed at runtime)
 * `Number of Stacks`: `Max Amount` will be the number of stacks that can be added.  
 * `Total amout of items (shows current/max)`: `Max Amount` is an numeric hard cap, items in depot will show (current/max) and a progress bar of how full the entry is.  
 * `Total amout of items only shows current`: `Max Amount` is an numeric hard cap, entries will show current amount and a progress bar of how filled the entry is.  
 * `None (no progress bar)`: the numeric hard cap is the INT32_MAX (`2.147.483.647`), entries will show current amount but no progress bar will be shown.  

`Max Amount`: works with `Max Type` (can be changed at runtime).  
`Can Drag Items To Inventory`: Controls whether or not a player can drag a stack into his own personal inventory (can be changed at runtime).  
`Can Be Used When Building`: Controls whether or not this depot can be directly used when crafting or building (can be changed at runtime, player can also turn this off for themself).  
`Is Player Specific`: If checked, each player will have its own specific contents for this depot.  
Savegame:  
`Persist In Save Game`: If checked, contents of this depot will be persisted in the same game, otherwise the content is lost.  

NOTE: if both `Can Drag Items To Inventory` and `Can Be Used When Building` are unchecked, then the depot will be read-only for the player, they can see it but cannot take anything out.

NOTE: Its currently not posable to Drag & Drop items from your inventory into a custom depot, they will instead go to the Dimensional Depot if its available, but this functionality can be added later if there is enough desire for it.

### Manipulating the Depot
<TODO INSERT IMAGE>
This mod exposes the Additional Depots Server Subsystem, that can be obtained in blueprint as shown below, or in cpp like `AAdditionalDepotsClientSubsystem::Get(world)` or `AAditionalDepotsServerSubsystem::Get(world)`. The client subsystem is mostly used by the ui to display the depots and correctly take them into account when constructing things. To actually alter the contents of a depot the Server Subsystem should be used only.

#### Server subsystem
The server subsystem exposes the follow methods, and should be accessed from server code only.  
`SetDepotContent`: overrides the current content of a depot with the specified items/amounts.  
`SetItem`: set the amount for a specific item in a specific depot.  
`Add/RemoveItem(s)`: Increments and decrements the amounts for specific item(s).  
`GetItems`: the current item/amounts in a specific list.  
`UpdateCanDragToInventory`: allow or disallow dragging to inventory.  
`UpdateCanBeUsedForBuildingAndCrafting`: allow or disallow this depot to be used for building.  
`UpdateMaxAmount`: Allows the maximum stored amount to be changed at runtime (does not truncate item amounts over the maximum).  