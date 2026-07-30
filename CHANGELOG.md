# [0.10.0](https://github.com/antwika/antwika/compare/v0.9.0...v0.10.0) (2026-07-30)


### Bug Fixes

* **apps:** catch in main so a failed run actually unwinds ([af4c12e](https://github.com/antwika/antwika/commit/af4c12eceebab8e31fb805534c10f01203518ad5))
* **apps:** declare the rule of five where a reference member is held ([bd73d50](https://github.com/antwika/antwika/commit/bd73d5023a3a63c28a215308aefe2ca746a77884))
* **ci,scripts,log,time:** fix badge race, add CI timeouts/caching, and three small bugs ([0731f1b](https://github.com/antwika/antwika/commit/0731f1bba5a7c4f7e406ac9cb02dfc2e89efb4db))
* **ci:** install the default configuration before a backend one ([1ecee55](https://github.com/antwika/antwika/commit/1ecee55d783d031704a15fdfb3ecea0b43a97eb1))
* **deps:** re-resolve conan-raylib.lock for raylib 6 ([a99f39b](https://github.com/antwika/antwika/commit/a99f39bdb517e123749f2f304eef0575f00a8440))
* **devcontainer:** add libxcb-util-dev, which xorg/system also demands ([8d8fa93](https://github.com/antwika/antwika/commit/8d8fa9323738d1109e4e9ecff0e6fa47ab9d151d))
* **ecs:** throw EcsError on entity exhaustion instead of std::exit ([9e54ffc](https://github.com/antwika/antwika/commit/9e54ffca3e63cfedc80d7558f0919779d13a23ac))
* **game,life,task_worker,sudoku,wfc:** reject malformed input at five app/library boundaries ([43f05dd](https://github.com/antwika/antwika/commit/43f05ddfad7a2e817d4a3fd15baa14a03f635f2f))
* **gfx:** make a backend's event queue always reach the end ([4c2a111](https://github.com/antwika/antwika/commit/4c2a1116a5626dab5cd4f4dd0b3dad0ec88670bb))
* **replay:** stop ScratchFile's destructor from being able to throw ([64bc3d0](https://github.com/antwika/antwika/commit/64bc3d072681d702b6a3b59ca4de8acc38a15b7a))
* **scheduler:** validate dependsOn before taking ownership of a job ([5eaa4cf](https://github.com/antwika/antwika/commit/5eaa4cf766c35ba0d7c27731ce990c2f4c417dfe))
* **scripts:** check_unused_test_doubles.py matches real #include directives ([6bd5cea](https://github.com/antwika/antwika/commit/6bd5cea3983e862f0b32b17ad0d399a15d9bef00)), closes [#include](https://github.com/antwika/antwika/issues/include) [#included](https://github.com/antwika/antwika/issues/included) [#include](https://github.com/antwika/antwika/issues/include)
* **task_worker,ecs:** close the remaining View.hpp coverage gap for real ([ee2f859](https://github.com/antwika/antwika/commit/ee2f8591b1aad44fbc9648d60da126cdf8a1232b))
* **wfc,ecs,replay:** fix three critical correctness/robustness bugs ([1d71a03](https://github.com/antwika/antwika/commit/1d71a03f197abfc1b8311fd4a14c21928c3c2d9a))


### Features

* **game:** draw the grid, the paths and the walkers ([eb78a55](https://github.com/antwika/antwika/commit/eb78a5524e408bb250f557642a884431a768f575))
* **game:** lay paths, drop walkers, and walk them ([53557cf](https://github.com/antwika/antwika/commit/53557cf0c6a7d5acf93dee9ba84b1e76d61ea02f))
* **game:** start empty and keep running ([4f2554f](https://github.com/antwika/antwika/commit/4f2554f87ef9ec98cf9f3b195da8d58aa3c243c5))
* **game:** the isometric projection and the walking rule ([c9d841e](https://github.com/antwika/antwika/commit/c9d841e8476f1b951f66c7faca6efb441ec0dfe1))
* **game:** wire the isometric grid into the binary ([2776055](https://github.com/antwika/antwika/commit/277605541c9614acff944dc92e2b873f61c15f5f))
* **gfx_demo:** draw the demo through antwika::ui ([7ea36aa](https://github.com/antwika/antwika/commit/7ea36aa4126dc959ce4698136d5f8b667d7864cd))
* **gfx_demo:** draw until the window closes instead of for three frames ([ef1bff4](https://github.com/antwika/antwika/commit/ef1bff4a56c503242344764ac53847b9e990d353))
* **gfx:** add the backend-agnostic graphics abstraction ([fd02f55](https://github.com/antwika/antwika/commit/fd02f5502d845d3be41cea3b9a04b9ac29d2fdd9))
* **gfx:** add the raylib backend, and let a backend own one window ([e50abea](https://github.com/antwika/antwika/commit/e50abea9de4776a8c780b8722acc21c5630542eb))
* **gfx:** add the SDL3 backend, and give window events an identity ([70af4cc](https://github.com/antwika/antwika/commit/70af4cc9cc937e45a29363c44a365e5de1ca21dd))
* **gfx:** draw lines ([166737b](https://github.com/antwika/antwika/commit/166737ba718eed0aab9e59e08b0689b756bd2df2))
* **gfx:** draw text through one built-in fixed-cell font ([d675091](https://github.com/antwika/antwika/commit/d675091af98154faf8a78ac26ca97131c9880088))
* **gfx:** paint drawText in the sdl3 and raylib backends ([30bb339](https://github.com/antwika/antwika/commit/30bb33963cf37f4765c830d8862b7588f9f35417))
* **gfx:** render textures loaded from PNG files ([6dac7b9](https://github.com/antwika/antwika/commit/6dac7b9a9076ba88e6e207f35136a4365b41608d))
* **gfx:** select a backend at build time and demo it end to end ([bef6f58](https://github.com/antwika/antwika/commit/bef6f588658152bd2da6bd64068a06d0460b39e2))
* **holdem,poker:** add no-limit hold'em, evaluated bitwise into one number ([95d1032](https://github.com/antwika/antwika/commit/95d103249b34cc26eb61ccc9f2f251d4d8f1d9a7))
* **holdem:** describe a hand value the way a player would say it ([1a6bb7d](https://github.com/antwika/antwika/commit/1a6bb7db58ace012bda5f29c767a36cfd27a4f27))
* **holdem:** report what an action staked, raised over and left behind ([2bc73e4](https://github.com/antwika/antwika/commit/2bc73e421844f16314a79962a32ccf671a625e98))
* **input:** add the vocabulary and the backend seam ([515ed47](https://github.com/antwika/antwika/commit/515ed472138ee7aea95e9ddc8540edb61b9ad0c6))
* **input:** fold edges into keyboard and pointer state ([2a016b8](https://github.com/antwika/antwika/commit/2a016b8eb7fcad53d32fa454607d873af46933c6))
* **input:** stop on a key, and thin out pointer movement ([91b07b3](https://github.com/antwika/antwika/commit/91b07b39a6dd34e899cc29195dc181f19aef0e18))
* **life:** draw a Board as rectangles ([1165694](https://github.com/antwika/antwika/commit/1165694aabef0ae5c7a1c871a1b730c80ca2ee33))
* **life:** draw on the board with the mouse ([4ea745a](https://github.com/antwika/antwika/commit/4ea745ae731ec2e24aba34377651e8861230f24c))
* **life:** hold the generations still while drawing ([f427ad7](https://github.com/antwika/antwika/commit/f427ad7c4583235445724ac41bb4d41be804dfee))
* **life:** let closing the window stop the run ([05ace66](https://github.com/antwika/antwika/commit/05ace66d4bbd47aca805454a1547c23ab86b3cea))
* **life:** pace the run so it can be watched ([4cf15af](https://github.com/antwika/antwika/commit/4cf15af1730f14bd5a22ee56a8f4c77826022387))
* **life:** read a Board without a Grid to map coordinates with ([151ac9b](https://github.com/antwika/antwika/commit/151ac9bb5630ba83162d74b87ef3a4ebdaae0062))
* **life:** render the board into a window each tick ([25a8fef](https://github.com/antwika/antwika/commit/25a8fef383b1417a85766c81e0882ff5daf4a881))
* **life:** show the board in a window ([40d415c](https://github.com/antwika/antwika/commit/40d415cf9543c1559b527e30bab213db752f80fd))
* **poker:** draw a table snapshot ([2381777](https://github.com/antwika/antwika/commit/2381777856f0559a55de1894fa3bd8faa1f0bfe7))
* **poker:** draw each tick, and stop when the window closes ([25cdaa4](https://github.com/antwika/antwika/commit/25cdaa40e719ecf682db8292015b505a6817e2ac))
* **poker:** open a window and draw the game each tick ([7da173f](https://github.com/antwika/antwika/commit/7da173fbaa06ecd57b7e90f90ae35020897df561))
* **poker:** snapshot what a spectator may see of a table ([6572e1d](https://github.com/antwika/antwika/commit/6572e1d6d11039ef8a8211511dff2116392681ed))
* **poker:** write every hand out as a hand history ([68220df](https://github.com/antwika/antwika/commit/68220df629a34c46d930aa1a048e20b5cb7a6b72))
* **replay,apps:** add JSON replay codec alongside binary ([8bf3b32](https://github.com/antwika/antwika/commit/8bf3b32e5409905b6f5f92d4acde457d0de5c555))
* **scheduler:** let schedule() take ownership of a job ([8c01554](https://github.com/antwika/antwika/commit/8c0155452663211277fb82c9016fa30a86978b4e))
* **time:** add an injectable sleeper ([85e3663](https://github.com/antwika/antwika/commit/85e3663fc84ec82cc1f7c43cd8d1ccc913e7c8cf))
* **ui:** add an immediate-mode UI library with nestable layouts ([1596961](https://github.com/antwika/antwika/commit/15969614e7c87fe4de0f4460450a7670a08d02db))
* **vscode:** choose the gfx backend once, then build it with Ctrl+Shift+B ([979eae8](https://github.com/antwika/antwika/commit/979eae8ef458ef7e852e1a3f42ed14d12edfff33))
* **vscode:** re-resolve every conan lockfile from one task ([d7736cb](https://github.com/antwika/antwika/commit/d7736cbb2b009368ed11d7eb85d172e3bb29a86b))

# [0.9.0](https://github.com/antwika/antwika/compare/v0.8.0...v0.9.0) (2026-07-29)


### Features

* **engine,replay:** run until an engine.stop event, not a fixed tick count ([4342036](https://github.com/antwika/antwika/commit/4342036a462a1527a861b713adcdb0a2e4bb8821))


### Performance Improvements

* **build:** parallelize cmake builds with -j24 ([ef41564](https://github.com/antwika/antwika/commit/ef41564b1461f5a95b2ed0c320153c0114217b41))

# [0.8.0](https://github.com/antwika/antwika/compare/v0.7.0...v0.8.0) (2026-07-28)


### Bug Fixes

* generalize gcovr main.cpp exclude, rename task-worker to task_worker ([0ee6228](https://github.com/antwika/antwika/commit/0ee622859798f8e7ec8f2549af7cc15768b1a9c2)), closes [#1](https://github.com/antwika/antwika/issues/1) [#2](https://github.com/antwika/antwika/issues/2)
* **task-worker:** harden task submission validation, add live task status ([c9ce2df](https://github.com/antwika/antwika/commit/c9ce2dfc4c2500666b7f20ea7bdfb31c2fe125ef))
* **wfc,sudoku:** validate solver weights, close Domain/CLI edge cases ([8978426](https://github.com/antwika/antwika/commit/897842660866eb10849a200217af98ecc14da2af))


### Features

* **scheduler:** add antwika::scheduler library ([6feb676](https://github.com/antwika/antwika/commit/6feb676ac7a71423bd5c4ee1f2fc192a3595c586))
* **sudoku:** add apps/sudoku showcase for antwika::wfc ([c7edb5d](https://github.com/antwika/antwika/commit/c7edb5def2092d3e624aa71f6e1bfd4c4ccb26eb))
* **task-worker:** add apps/task-worker demo app ([9833a2d](https://github.com/antwika/antwika/commit/9833a2dfacf29653f79ad19ab3d3705d5cb60117))
* **task-worker:** merge worker/task status prints under one tick header ([3c2d261](https://github.com/antwika/antwika/commit/3c2d26147472e988ed127b5d1759a83f39617a43))
* **task-worker:** show task dependencies, run demo 2 more ticks ([381a2dc](https://github.com/antwika/antwika/commit/381a2dc65cc950417e1edfbaac0b311409b3a785))
* **wfc:** add antwika::wfc library implementing deterministic WFC ([243287d](https://github.com/antwika/antwika/commit/243287d659c18a78bfb0f50e13ec2aa690a534ca))

# [0.7.0](https://github.com/antwika/antwika/compare/v0.6.0...v0.7.0) (2026-07-28)


### Features

* **ecs:** add double-buffered ComponentStorage<T> ([c503be2](https://github.com/antwika/antwika/commit/c503be2fdf1caaa7ff0d3850ac822f30e5ca3aa3))
* **ecs:** add ISystem and SystemScheduler with phases ([d6cfa9c](https://github.com/antwika/antwika/commit/d6cfa9cb5b4dd0a9c1c98eac0d3e24a492db1971))
* **ecs:** add View and World ([24ec64a](https://github.com/antwika/antwika/commit/24ec64a1ba9496c4005e91a3a6477f7821b1a2e3))
* **ecs:** scaffold library with Entity and EntityManager ([54392b5](https://github.com/antwika/antwika/commit/54392b582cdc66f028c9aae455d5db7bd8e642ba))
* **life:** add Game of Life demo app on antwika::ecs ([81983f0](https://github.com/antwika/antwika/commit/81983f04284057c0474ee70927ea489b7dbb15a3))
* **reducer:** add generic IReducer/ReducerSink library ([e42e387](https://github.com/antwika/antwika/commit/e42e387e1e5fd5eeed6ca7ef20f9a3eeae5efc2c))

# [0.6.0](https://github.com/antwika/antwika/compare/v0.5.5...v0.6.0) (2026-07-27)


### Features

* **engine:** add fixed-timestep step() and built-in tick event ([e3b3890](https://github.com/antwika/antwika/commit/e3b38905f5c26ec6a244e84c6fc7bcd74b672b11))
* **event:** add opaque payload field to Event for extensibility ([909a024](https://github.com/antwika/antwika/commit/909a024a818c6089a5d5a88b0dd8063e6ba45456))
* **event:** add TickedEventDispatcher to tick-stamp dispatched events ([effb8d3](https://github.com/antwika/antwika/commit/effb8d35f337bb31dbe37aac55f3b18cffef8e0b))
* **event:** add TimedEvent, ITimedEventSink/History, ReplayRecorder ([30131c7](https://github.com/antwika/antwika/commit/30131c77f5cb502f925030a4cd6a379e40b9a776))
* **game:** add GameState and GameStateReducer example ([7662125](https://github.com/antwika/antwika/commit/76621256f1ba15857cdfb337f7baa4b71bd024a9))
* **game:** wire record/replay CLI flags into bootstrap and main ([5d4e347](https://github.com/antwika/antwika/commit/5d4e34717f2471ffb9a29a627a93e96d8292e816))
* **replay:** add ReplaySource and EngineLoop orchestrator ([9529b22](https://github.com/antwika/antwika/commit/9529b222400ccd1a4354f6bc2e26e30ce736f174))
* **replay:** add versioned binary replay writer/reader ([140213d](https://github.com/antwika/antwika/commit/140213dca7b6ba02073261b0a8460a96d004a993))
* **replay:** scaffold replay lib with binary event codec ([aa3b5e2](https://github.com/antwika/antwika/commit/aa3b5e23d19ede13cd00b12a9d8964bc19347072))
* **time:** add fixed-size Tick type ([d98705b](https://github.com/antwika/antwika/commit/d98705ba1a003922a8375bfc79e81ff294203a58))

## [0.5.5](https://github.com/antwika/antwika/compare/v0.5.4...v0.5.5) (2026-07-27)


### Bug Fixes

* adjust badge generation to be more harmonized ([5c6d49e](https://github.com/antwika/antwika/commit/5c6d49ea1397142c5337c8e56e556fe74a2cdd04))
* try use an img.shield.io badge for ci ([dff447d](https://github.com/antwika/antwika/commit/dff447d4af6f62a9d8209de1596ec7358fa836ef))

## [0.5.4](https://github.com/antwika/antwika/compare/v0.5.3...v0.5.4) (2026-07-27)


### Bug Fixes

* various issues with coverage, scripts ([be14e91](https://github.com/antwika/antwika/commit/be14e91316a775cb1dd094e7e0361adddb19a543))

## [0.5.3](https://github.com/antwika/antwika/compare/v0.5.2...v0.5.3) (2026-07-26)


### Bug Fixes

* add [[nodiscard]] to pure query methods ([49e51cb](https://github.com/antwika/antwika/commit/49e51cb9bbb609f55c0454304adedfb87576c8eb))
* add a non-throwing logger failure fallback ([a2ed2ec](https://github.com/antwika/antwika/commit/a2ed2eced6bc85e53a2b414cff25ddb25cb5051d))
* add missing include ([3c00940](https://github.com/antwika/antwika/commit/3c00940f491d2de8fe4555665e24bbdf334f5f5d))
* add missing include ([17a9d72](https://github.com/antwika/antwika/commit/17a9d722b6c5ba4a9cd5d7ecd93b231619164731))
* add missing override keyword ([cfbe9e0](https://github.com/antwika/antwika/commit/cfbe9e01dbf8b06e807a3b480bc3ed960d8022d7))
* align rule-of-five semantics with other DI classes ([a4ca0ec](https://github.com/antwika/antwika/commit/a4ca0ec85d84dd06484be1343e06f8246d25559a))
* avoid unnecessary copy when dispatching events ([f90ad10](https://github.com/antwika/antwika/commit/f90ad10b0bd7fbc3cba6c72a7de0179e0c9d86d9))
* c++20/23 features already supports relational operators for enum ([cfc9dd8](https://github.com/antwika/antwika/commit/cfc9dd8a32fd5f243d632ce7f43f9c1f3906e7d5))
* enable strict warnings, close untested-mock regression gap and other various fixes ([3000d3b](https://github.com/antwika/antwika/commit/3000d3b21bd9a797322b8d2fd47be5c8a76f069f))
* make formatter interfaces const-correct ([2dfbb21](https://github.com/antwika/antwika/commit/2dfbb219fa940e11724a47e96d5c0b57b3bea1f4))
* make Level toString return std::string_view to be allocation-free ([7bda170](https://github.com/antwika/antwika/commit/7bda17021a6029f1ce6fad113eb1ae099bfb6980))
* make reference-member classes non-assignable ([a43d36c](https://github.com/antwika/antwika/commit/a43d36c24dcfe108785f4496b70866520520964e))
* regresseion, add missing override ([32f36fd](https://github.com/antwika/antwika/commit/32f36fd864990fc71a75f1245f8c3c4cd5f22a14))
* remove global using declarations from include headers ([117307f](https://github.com/antwika/antwika/commit/117307f85180b06a24e597f28d06b3c48ec554af))
* remove stale MockEventQueue::getHistory (no longer exists in IEventQueue) ([39474f7](https://github.com/antwika/antwika/commit/39474f79a9a2f4352f4b5d5c0db7e1807c828e49))
* remove unused variable ([8a14881](https://github.com/antwika/antwika/commit/8a148819a4147907ccd0dd599d4d98b915233d17))
* the equality operator can be defaulted since c++20 ([a7e6bbc](https://github.com/antwika/antwika/commit/a7e6bbc304603bcfc1a6af933bbb9077fa99b069))
* use event dispatcher in the Game implementation ([092f7fb](https://github.com/antwika/antwika/commit/092f7fb22a6445092487e5a172136b9d5ba34ed2))

## [0.5.2](https://github.com/antwika/antwika/compare/v0.5.1...v0.5.2) (2026-07-26)


### Bug Fixes

* **ci:** only expect antwika_game.exe for mingw builds ([cd5beb9](https://github.com/antwika/antwika/commit/cd5beb9268a9563ec1b166ad4dfb5999d007c653))
* harmonize cmake files and naming conventions ([7564148](https://github.com/antwika/antwika/commit/7564148848a9dfccbe3046aa94e57143bac163c6))
* move all source code into src/ and simplify all CMakeFiles.txt ([209f300](https://github.com/antwika/antwika/commit/209f3008865760f43abf811a12dcda3d2cd0c965))

## [0.5.1](https://github.com/antwika/antwika/compare/v0.5.0...v0.5.1) (2026-07-25)

# [0.5.0](https://github.com/antwika/antwika/compare/v0.4.0...v0.5.0) (2026-07-25)


### Features

* add ILogPolicy ([c43a9e3](https://github.com/antwika/antwika/commit/c43a9e30fb6ef472e8ada79d437c30e68a124897))

# [0.4.0](https://github.com/antwika/antwika/compare/v0.3.2...v0.4.0) (2026-07-25)


### Bug Fixes

* now dev container welcome message only prints first line from gcov --version output ([011ef27](https://github.com/antwika/antwika/commit/011ef276ee2690441bd06b99dbb6dd6b62ceab51))


### Features

* add lib/event and an event queue is processed upon engine start ([59c8721](https://github.com/antwika/antwika/commit/59c87216be9c450e81ba0ae576af16ace823a266))

## [0.3.2](https://github.com/antwika/antwika/compare/v0.3.1...v0.3.2) (2026-07-25)


### Bug Fixes

* **ci:** build apps now has parenthesis ([4b8dbbe](https://github.com/antwika/antwika/commit/4b8dbbebb3eda973dea76db23751f9fa45598f0f))

## [0.3.1](https://github.com/antwika/antwika/compare/v0.3.0...v0.3.1) (2026-07-24)


### Bug Fixes

* **base:** adjust info and welcome indentation ([941d01d](https://github.com/antwika/antwika/commit/941d01db3d3cae23ea6c09a5af0fd390f2d36e0c))
* **deps:** use c++23 ([9b0c881](https://github.com/antwika/antwika/commit/9b0c881280ee0b0a5b45705761813b098f3e8d96))
* rename apps/app to apps/game ([f483343](https://github.com/antwika/antwika/commit/f483343fe231e1cc253341058a4b1e73d4618d79))

# [0.3.0](https://github.com/antwika/antwika/compare/v0.2.4...v0.3.0) (2026-07-24)


### Bug Fixes

* **ci:** build the application with each dev container image ([70fbfa1](https://github.com/antwika/antwika/commit/70fbfa1019f31730dfc1e721c4224ac5c0e6ed9e))
* coverage report task and various tests in libs ([47b074b](https://github.com/antwika/antwika/commit/47b074be2031fbfdee0e170c771d2f9057fc26ab))


### Features

* add support for coverage report ([f46083c](https://github.com/antwika/antwika/commit/f46083c8d57c1cbbc64ecea3e425da74d40f7df5))

## [0.2.4](https://github.com/antwika/antwika/compare/v0.2.3...v0.2.4) (2026-07-24)


### Bug Fixes

* **ci:** print the released version in job output ([55d9ff5](https://github.com/antwika/antwika/commit/55d9ff5033554f17d6ba47b4f680fd4f0af66119))
* **ci:** specify default branch main, and do not warn about detached HEAD state ([8a962df](https://github.com/antwika/antwika/commit/8a962df29f07c6cd91699f3b999722e0c15e4d91))

## [0.2.3](https://github.com/antwika/antwika/compare/v0.2.2...v0.2.3) (2026-07-24)


### Bug Fixes

* **ci:** fix image latest tag condition ([d17ae59](https://github.com/antwika/antwika/commit/d17ae592fe779406c658dc3797e4f1db0c62dcfb))
* **ci:** rework the ci and release workflows ([acae28c](https://github.com/antwika/antwika/commit/acae28c9d00adcf4f828bdafaaa831a3ef07e241))

## [0.2.2](https://github.com/antwika/antwika/compare/v0.2.1...v0.2.2) (2026-07-24)


### Bug Fixes

* **ci:** install required plugins for semantic-release ([f8175f8](https://github.com/antwika/antwika/commit/f8175f87c031fb24ec059718e7020e3519bf6591))
* **ci:** make semantic-release update changelog.md and release to github ([48e7f55](https://github.com/antwika/antwika/commit/48e7f55429fa9f368f9d727a7e56163370690ff3))
