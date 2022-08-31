# BunjilandNeo Architecture

I am using a modular architecture whereby to the extent possible, actor functionality is encapsulated within its own reusable actor component.

The following is an account of the main architectural features, in principle kept constantly up-to-date.  For convenience, changes are usually committed at the same time as changes to related code.

## Persistent Level Structure

Bunjiland makes use of a so-called persistent level, which runs in the background to handle loading and unloading of sub-levels (the actual game maps) and, eventually, any other level-specific logic.  Under `Project Settings` -> `Maps & Modes`, both the `Editor Startup Map` and the `Game Default Map` are set to the game's persistent level, `GameLevel`.

Levels themselves are stored in `./Content/Bunjiland/Levels`.  Each level has its own subfolder, which contains the level `.umap` itself, and any other files which are intrinsically tied to that level, such as level sequences, etc.  Level assets that are shared (or potentially shared) between levels - such as material instances (not master materials) and brushes - are under the `Shared` subfolder here (and potentially their own sub-subfolder).

## GameInstance

There is a custom game instance, `BP_BunjilandGameInstance`, which I am using principally as a global variable store - initially for things like debug environment variables (e.g., the `SkipIntroMenu` boolean allows you to skip the start screen and menu so you can get straight into the game for testing purposes).

## Player-related

### BP_BunjilandPlayerCharacter

Bunjiland started off using the third person UE template.  That template provides you with a blueprint character class, subclassing the main engine base player C++ character class, `Character.cpp`.  You then subclass that blueprint version to your own blueprint version and take it from there, which I did, with a character class I (confusingly) named `BP_ThirdPersonCharacter`.

That is a great way to get started quickly, but as development progressed some cracks appeared.  Mostly annoying was that I couldn't remove the default camera, which I wanted to replace with a cine camera component, and I couldn't for the life of me figure out where the input controls were located.  Opening the base C++ class in Rider that I had subclassed for my blueprint version, I discovered that the input controls are actually on this custom base C++ class, and are therefore not visible on the blueprint class.  This is annoying.

So I designed a new custom character, subclassing the main engine base `Character.cpp` class instead of the custom C++ character provided by the template.  This meant that I needed to re-implement the (now-missing) input controls.  What I discovered, however, is that if you add the `ThirdPerson` content starter pack to your project, the player character blueprint in that project - also named `BP_ThirdPersonCharacter` - has the input controls _in the blueprint_, instead of in the C++ class it subclasses.  So all I had to do was copy them across to my new player character (more specifically, to the actor component handling movement that is attached to that player character).

I then added the the following actor components to this blueprint:

- MonologueFlowPlayer (an `ArticyFlowPlayer`): to provide for player character "monologues".
- AC_CharacterFlowPlayerHandlerComponent: A helper component that must accompany any instance of the above `ArticyFlowPlayer` component, be it on this player character, or on any other entity derived from `Character.cpp` that also has an `ArticyFlowPlayer` attached (at this point, just NPCs).  The purpose of this actor component is to catch and handle the output of its sibling `ArticyFlowPlayer`, mainly processing and storing text and branch objects and providing custom public functions from which they can be retrieved.  Aside from the text processing, the main reason this is currently required is that each instantiated `ArticyFlowPlayer` immediately "plays" at `BeginPlay`.  If I can figure out how to suppress this behaviour then I might be able to find a different approach (e.g., foregoing the custom handler functionality entirely and putting the text processing on `asdf` instead).  Note also that at present this is derived from a blank actor component, but that in future it could directly subclass `ArticyFlowPlayer` and just add the required functionality, but this would need to be done in C++.
- AC_PlayerCharacterInputComponent: Handles movement input on the player character.  I decided to keep this on the player character instead of the controller (where it is sometimes kept) because it is quite bound up in the physical mechanics of the player character pawn anyhow, and would change if, e.g., the player entered a vehicle.
- AC_PlayerCharacterCameraContolComponent: Provides camera controls specific to this game and this (cine) camera.

#### Player Camera

A note on the player-attached tilt shift camera.  I adapted the setup from [this](https://www.youtube.com/watch?v=XAtzQc9-0wY&t=240) YouTube video.  A realistic depth-of-field effect comes from the interplay between `Min FStop` and `Current Aperture`.  `Current Focal Length` does not seem to bear on it.  Steps are as follows:

- Start with a default Cine Camera
- Set the `Current Focal Length` to 24 (actually, I prefer 18; seems to flatten the image too much)
- Set the `Min FStop` to 0.001 or even 0
- Assuming that the camera is attached directly to the end of a spring arm, set the `Manual Focus Distance` to a value equal to the length of that spring arm
- Enable `Draw Debug Focus Plane`, even though I still haven't figured out how to directly preview the camera in-editor
- Finally, lower the `Current Aperture` to between 0.03 and 0.1; lower for more extreme tilt-shift effect.  I've settled on 0.09 for now; I found that 0.06 was too extreme.
- I quite like the amount of blur that I've got with the current settings but I would like a slightly smoother transition threshold between clear and blurry.  It's not clear to me how the FStop value interacts with the `Current Aperture` value, but changing it doesn't appear to obviously affect the visuals.  Perhaps it's something to experiment with in future.  Note that there is a discrete `Depth of Field` option on the `Post-Process` -> `Lens` settings of the camera component so might be worth experimenting with that too.

Note that if you just want to change the length of the `TiltShiftCineCamera` spring arm (i.e., the distance of the camera from the player), you need to adjust not only the length of the spring arm, but also the `Manual Focus Distance` of the camera itself: those two numbers must match.

I've naively implemented manual focus controls to keep the player character in focus as the camera collides with pieces of scenery.  There are two problems with the current implementation: the check is currently run on every event tick, instead of triggered by a component overlap event.  I couldn't figure out how to get this to work on the camera or spring arm.  Second, although after changing the spring arm collision channel to “Visibility”, the camera no longer collides with NPCs, it does still collide with other dynamic actors, in addition to the scenery.  It should not, I think, collide with dynamic actors.  I need to figure out collision settings propery.

##### Controls

The following was guided primarily by [this](https://forums.unrealengine.com/t/how-limit-spring-arm-pitch-during-rotation/396304).

We constrain the camera pitch using the following settings on the camera _spring arm_.

First, ensure that `Rotation` under `Transform` isn't set to `Absolute Rotation` - this only works with relative rotation.

<img src="./Images/Screen Shot 2022-08-14 at 14.57.21.png" alt="isolated" width="400"/>

Next, ensure that `Use Pawn Control Rotation` is enabled, but that `Inherit Pitch` is disabled.  This will allow you to rotate the camera without having to specify manual controls, and then allow us to specify manual controls just to control _pitch_, which is what we want.  Doing it this way allows us to constrain maximum and minimum pitch values that prevent the player from moving the spring arm (and the camera attached too it) too far up or down.

<img src="./Images/Screen Shot 2022-08-14 at 14.57.36.png" alt="isolated" width="400"/>

Finally, we build the camera pitch control and constraints as follows:

<img src="./Images/Screen Shot 2022-08-14 at 16.04.44.png" alt="isolated" width="400"/>

##### Mouse Wheel Zoom

I have set up naive mouse wheel zooming, including clamps at either end to prevent over and under-zooming.

To do this, following hints online, I created a new input axis targeting the mouse's scroll wheel, called `Scroll Up/Down Mouse`.  When set to 1, this represents a "non-inverted" scroll wheel; whereas -1 means "inverted".  Note that in practice it will interact with your system settings so that, in my case as I use an inverted "Mac-style" mouse, an inverted setting here in `Scroll Up/Down Mouse` would actually get me an overall non-inverted setting as far as the game is concerned.  I had intended to control the invert setting with a global variable on the game instance, which I have called `MouseWheelInvert`, but it's not yet implemented.

The actual player camera zoom controls are then implemented on the player blueprint, currently `BP_ThirdPersonCharacter`.  As all input - regardless of device - seems to be checked every frame, there is first a gating check that checks for a non-zero input value (i.e., checks whether the mouse wheel is actually moving); if there is one, it proceeds to either add or subtract, as the case may be, the target boom length for the cine camera on the player.

### BP_BunjilandPlayerController

This subclasses the normal player controller class.  I am using this class mainly to hold player UI-related components (which includes any UI components that handle player HUD, game menus and - importantly - any dialogue from Articy).

Two actor components are used:

- AC_ControllerUIMasterComponent: Handles all essential UI-related logic for the relevant player, other than that concerning Articy objects.
- AC_ControllerUIDiscourseComponent: Handles all Articy-related logic.  The choice of "discourse" instead of, e.g., "dialogue", is intended to reflect the fact that it handles both player monologues and NPC dialogues.

Note that the UI itself (as opposed to the logic that controls it) is implemented as a series of Widget Blueprints, located at `./Content/Bunjiland/Core/UI`.

I did consider making use of Unreal Engine's `HUD` class, but it didn't really seem that useful to me, seeming to favour non-interactive UI's, for starters, but also having a weird API.  Instead, having encapsulated UI logic into a separate actor component that lives on the player allows, I hope, for better modularity and perhaps even re-use across projects or with multiplayer.

### Inventory System



## Articy: Draft Integration

Game dialogue is handled via integration with the Articy: Draft UE import plugin.  That plugin leaves all of the UI integration work un-done.  I have implemented it as follows.

#TODO: To be updated

- Upon receiving the `Interact` input, checks to see whether (i) there is already a dialogue window visible (in which case it does nothing further), then whether any actors overlapping the capsule collision component on the player character are of the type `ArticyFlowPlayerMeshExample`, following which it will cast to that type and then save a bunch of relevant properties to in-scope variables.
- It then adds the dialogue UMG widget to the viewport, and immediately saves some useful properties of the nested UI widgets into in-scope variables.
- It then immediately binds an event to a series of nodes that will remove the entire dialogue UI from view if the player walks away from the actor, and calls `Set Input Mode Game and UI`.
- It then calls `Update Articy Objects and UI`, which is the key function here.  That function calls a bunch of functions on the `ArticyFlowPlayerMeshExample` actor which return the stage directions, speaker name, current dialogue and branches from the `ArticyFlowPlayer` component then passes them to `Dialogue UI Text Window` widget, which contains the actual logic for drawing them to the UI.
- Key concepts to understand here are how the branches and buttons work together; I recommend reading the documentation for the [Articy UE import plugin](https://github.com/ArticySoftware/ArticyImporterForUnreal#setup).  Of note, while I expected that I would have to design my own button logic, Articy seems to have included an (undocumented) debug button which contains most all of the logic you need, known as `W_ArticyBranchButton` (located under the `Engine` folder, not under the `Content` folder).  I just adapted it, mainly by replacing references to the debug flow player with a normal flow player, and adding two event dispatchers for updating and removing the dialogue UI.
- Finally, the level blueprint loops over all of the branch buttons that are now instantiated, and binds the actual UI update and removal logic to the now-exposed event dispatchers.  Every time a button is clicked, the `Refresh Dialogue UI` event is called.  Once there's no more conversation to be had, a final click will remove the dialogue UI altogether and clear the stored variables.

### Articy Project Settings Paradigm

Additionally, you need to ensure that both the Articy project and the Articy Flow Component (on any relevant actors - e.g., NPCs) are correctly configured.  Namely:

- In Articy, use arbitrarily-nested flow fragments to organise everything, and use dialogues to contain all dialogue fragments.  Make sure that _all dialogue fragments have in and out pins with the dialogue within which they are contained_.  But it is not necessary to have in and out pins running from a flow fragment to any dialogues within it, as you won't be pointing your Articy Flow Player components at the flow fragments at all.
- Follow the project paradigm that I have set up, which currently is to use arbitrarily-nested flow fragments to organise dialogues by Location -> Story (S0x) -> Episode (E0x) -> Part (P0x), with the following applying:
  - Stories are complete self-contained stories;
  - Episodes are discrete conversations or scenarios that occur at one time period;
  - Parts are how you break up conversations logically for modularity - exactly how you do this depends on the conversation but, for example, "greeting", "request", "counter-request" might be suitable, allowing you to modularise conveniently for legibility.
- Note that because of the way dialogues (and dialogue fragments) are presented for selection in the UE editor, you need to nest the complete Location -> Story (S0x) -> Episode (E0x) -> Part (P0x) information into each flow fragment as you move deeper.  This is a pain at design time but makes it easier to select relevant dialogue from the editor.
- Further, variable _sets_ should be organised into sets prepended with "Dialogues" and attaching location and Story numbers; an example would be `Dialogues_Mycelia_S01` (note that you can't use a period `.` in variable set names).  The actual variables within those sets should simply refer to episod number along with a description, so for example: `E01_PlayerKayleSpoken`.  This level of organisation is called for because I expect to use Articy variable functionality extensively, but also because it allows you to actually be fairly slack with the way you name your variables - if they are already referrable to the Story (from the enclosing set) and the episide (from the variable name), then it won't be hard to come up with unique descriptors.
- Ensure that you use _in-pin_ conditions and instructions, instead of putting instructions and conditions in separate nodes.  For some reason, the latter just does not work.
- If you want to include narrative text (that is, descriptive text with no speaker), just leave the `Speaker` field empty in Articy and the UE integration will understand that to be narrative text.
- The setup should look like this:

<img src="./Images/Screen Shot 2022-08-15 at 7.19.02.png" alt="isolated" width="400"/>

- In Unreal Engine, set the Articy Flow Player component on the relevant actor to "Pause On" only on “Dialogue Fragment” and, crucially, _set the “Start On” to point to a dialogue_ (not a flow fragment which, remember, is not connected to anything).

I might revisit the above if for some structural narrative reason it makes sense for a flow fragment to have connecting pins to the dialogues it contains, but as far as I know the only changes required there would be to add the connections and then configure your Articy Flow Player component to point to the flow fragment, instead of a dialogue, and the change "Pause On" to include flow fragments.

#TODO: A few things remain undone; here's all I can think of at the moment:

- The current logic to check overlapping flow player actors with the player character before initiating dialogue should choose the _closest_ flow player actor (or the one the character is looking at?) to interact with.  Some more logic will be needed to do that.
- There's currently no explicit integration with the global Articy database object.  That's no problem for the flow player itself, which already has access to those variables, but I also need direct access to handle certain game events.  That should be as simple as adding a reference to the database in the level blueprint in the manner set out in the Articy UE import plugin documentation.  The first such usage of that database connection would be to implement "broken conversation" mechanics so that NPCs react accordingly if you walk away from them mid-conversation. OR, alternatively, prevent the player from moving away mid-dialogue and make them explicitly end each conversation, in which case I could handle the "broken conversation" mechanics entire in-flow player without needing to hit the databse directly.
- I cannot get `Set Keyboard Focus` to automatically select the first button in the button box each time the dialogue flow player advances.  That means that you can't currently navigate dialogue without using the mouse.

## External Cameras

External cameras (i.e., any cameras that are not a component on the player character itself) are in `./Content/Bunjiland/Tooling/Camera`, along with related files such as blend trigger volumes, shake volumes, etc.

Currently, the camera blender functionality, and the level sequence player functionality, are contained within the `Map_Mycelia` level blueprint.  As these functions will ultimately be shared between levels, I need to find a more elegant place to store them.

## Master Materials

All master materials are in the own top-level folder at `./Content/Bunjiland/MaterialLibrary`.  These should never be invoked directly in-level.  We are using a "material instances only" policy.  Instances, which are created and then shared between levels as outline in the `Levels` section above.

## Game "Objects"

Really meshes and their associated actors.  They are stored in `./Content/Bunjiland/Objects`, then either `Natural` or `Manufactured`, and are then sorted further into subfolders.

## DCC Assets

Master DCC assets are stored in the `./Content/BlenderLinked` folder and are "linked" to Blender via the import tools.  What this means is that they cannot (at this stage) be used directly in levels, because doing so breaks the link with Blender, meaning they would need to be reimported.  So for now, assets in this folder aren't touched; instead, I make copies of the meshes and materials in this folder and then put them into the `./Content/Bunjiland/Objects` folder (alongside any relevant actors, etc), from where they may then be used in-level.