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
