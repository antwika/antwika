# [1.0.0](https://github.com/antwika/antwika/compare/v0.11.0...v1.0.0) (2026-08-02)


* feat(replay)!: a replay is JSON Lines, appended as the run goes ([12f8700](https://github.com/antwika/antwika/commit/12f8700d3bd35e1ce933c58e27a2ac7427a49660))
* refactor(i18n)!: give every module its own message ids and catalogues ([ad1ad74](https://github.com/antwika/antwika/commit/ad1ad742f6e7f4edfc20458c7b97cee4498b2433))


### Bug Fixes

* **apps:** six verified findings across companion, life, game and atlas_editor ([2404294](https://github.com/antwika/antwika/commit/24042945f94f443237edd916de448f15b069e4f5))
* **backends,notation:** input edges that arrive as typed (FINDINGS-3 4, 9, 10, 31, 11) ([5133de9](https://github.com/antwika/antwika/commit/5133de9f60c33587bb651215498e56bb48d4925d))
* **build,ci:** make the toolchain record, the asset copies and the release guard honest (FINDINGS-3 17, 18, 19) ([6e42d76](https://github.com/antwika/antwika/commit/6e42d7649f2ca3acf231bba09e055b1dd6087007))
* **companion:** refuse a saved companion that perished mid-sentence ([8639b24](https://github.com/antwika/antwika/commit/8639b249ab7ff058b1293c6344728456ed7d12d6))
* **ecs:** take the staged ops off World before running them ([69a0296](https://github.com/antwika/antwika/commit/69a02961ba17f92716df1b4780291ff5411bb52b))
* **game:** hold a walker still while the run is paused ([8a1caa3](https://github.com/antwika/antwika/commit/8a1caa3d5dbd0749e0dd717dec6f787910f432a5))
* **game:** refuse a saved zoom level past the end of the table ([3828d63](https://github.com/antwika/antwika/commit/3828d632577a7c24ab0a48aedb162fe846fe3678))
* **game:** refuse a step phase no walker could ever be in ([de65484](https://github.com/antwika/antwika/commit/de654842e0d40727c06807c834325a3c70900abc))
* **gfx,ui,input:** six verified review findings across the view stack ([83e2c4c](https://github.com/antwika/antwika/commit/83e2c4c1b7fa7389f0cbdc7833c27f8bf5c99342))
* **gfx:** anchor every glyph when the viewport scale is not whole ([e1ebe93](https://github.com/antwika/antwika/commit/e1ebe932d0ced6c3534af4a94ea42278c7a903c2))
* **gfx:** exclude the unwind landing pad glyphAtlasBitmap cannot reach ([796fc90](https://github.com/antwika/antwika/commit/796fc9003d3050e20b8fdddba0d61487745ed45c))
* **i18n:** keep the two adopting apps' coverage at 100% ([a4e5d06](https://github.com/antwika/antwika/commit/a4e5d06150912cb2acbb431cdddbc14740b60933))
* **i18n:** repair a doc comment split by the catalogue merge ([a149c22](https://github.com/antwika/antwika/commit/a149c221478570420cb524cb7b52011b5c33f4d7))
* **life:** walk the whole drag segment (FINDINGS-3 13) ([adb6502](https://github.com/antwika/antwika/commit/adb6502648f70dd8944fd7f5978555e93bd016cc))
* **music_editor,notation,wiki:** the music-stack tidy tier (FINDINGS-3 21-24) ([3354f4b](https://github.com/antwika/antwika/commit/3354f4b048bd27c112596e64c57d4bd56588eb3e))
* **network:** keep the frames a peer sent just before it closed ([f6e28c4](https://github.com/antwika/antwika/commit/f6e28c4cd16d0272bd80ffbe5c05008e7dfd7f0c))
* **pathfinding:** refuse costs that sum past what a Cost can hold ([18e3cf6](https://github.com/antwika/antwika/commit/18e3cf64341b66fee9aa51e2eb095a0fbac64e6a))
* **pattern:** move a fraction's sign to its numerator (FINDINGS-3 1) ([8f8549b](https://github.com/antwika/antwika/commit/8f8549b215ef691e9fbbcf9e78fed138a2e76394))
* **replay:** close four format holes in one pass (FINDINGS-3 14, 15, 16, 29) ([527abe1](https://github.com/antwika/antwika/commit/527abe178e93b30b29e0977c340afcd7e102dd27))
* **replay:** refuse documents nested past a small bound (FINDINGS-3 2) ([a00b0cc](https://github.com/antwika/antwika/commit/a00b0cca273b503b542516ce8fe54f325b6b50ca))
* **scripts,ci:** make the build tooling tell the truth about itself ([23d71e5](https://github.com/antwika/antwika/commit/23d71e5b277178ece8e008c687e75fdb2d16126e))
* **sound:** refuse a waveform with no frames in it ([d2e18da](https://github.com/antwika/antwika/commit/d2e18da187e84462c0dac1f0e655d1d8a479e318))
* **sudoku:** give each replay case its own scratch file ([70c8baa](https://github.com/antwika/antwika/commit/70c8baa81c2885c847c02e6137478998d516ff6f))
* **synth,notation,pattern:** the music stack's verified review findings ([1740b0a](https://github.com/antwika/antwika/commit/1740b0a6cdcb8abe4d83e470da9c82276d45fe63))
* **tween,pathfinding:** fit the guards to the arithmetic they guard (FINDINGS-3 5, 6) ([de780ae](https://github.com/antwika/antwika/commit/de780ae19fb1ef302134d37e0ee99bb64d25273e))
* **ui,music_editor:** scope a drag to where its press landed (FINDINGS-3 7, 8, 30) ([2f81d1c](https://github.com/antwika/antwika/commit/2f81d1c4f6f8b2b91d6b6a40d20ef039e8d954aa))
* **ui:** a pointer an overlay claims never reaches beneath it (FINDINGS-3 3, FINDINGS-2 11) ([e2333f6](https://github.com/antwika/antwika/commit/e2333f66ec2e09936917c372caef98a1a7d79f51))
* **ui:** keep children inside the box and refuse a half-built frame ([66dce21](https://github.com/antwika/antwika/commit/66dce21411ab5024956ccb82c6bf767c5dd1e784))
* **wfc,replay,ecs,pathfinding,engine:** the determinism core's edges ([95a9ea6](https://github.com/antwika/antwika/commit/95a9ea67e89de8e897db3748728840775ec8580b))


### Features

* **app:** a canvas pointer mapping, and a fullscreen toggle above the loop ([60bfc5e](https://github.com/antwika/antwika/commit/60bfc5eaa5e65d582c9b37e2f7d0841a5b210eb2))
* **assets:** bundle Roboto Mono as a shipped font asset ([f8bdfe9](https://github.com/antwika/antwika/commit/f8bdfe91f09b3472984629e3316a631ddeb3bdc9))
* **atlas_editor:** announce the opening sheet ahead of the recorder (FINDINGS-3 12) ([9fed779](https://github.com/antwika/antwika/commit/9fed779102ed7e19d3ad74eec5f43ef7827b187a))
* **atlas_editor:** word the toolbar and the status line from a catalogue ([e529a1d](https://github.com/antwika/antwika/commit/e529a1dea135344d86619cd58291971fb724c7cb))
* **companion,i18n:** name every prop in the box that presses it ([3d27e1f](https://github.com/antwika/antwika/commit/3d27e1f4186ac6d92c9492395a0258284880ccc0))
* **companion:** a breath eases rather than stepping ([b4ea31f](https://github.com/antwika/antwika/commit/b4ea31f4fee7e699ae6fdbc8c5020ad0ebc70aa4))
* **companion:** double the window and report the state in words ([53af213](https://github.com/antwika/antwika/commit/53af213029b69bf9aab20a1ba6f13c5bc86b23f6))
* **companion:** give it a speech bubble it says a fixed set of lines in ([7898e83](https://github.com/antwika/antwika/commit/7898e83663d09000c9dfdee9d32626b9e4331ec9))
* **companion:** make energy the life meter and give it three verbs ([ac95320](https://github.com/antwika/antwika/commit/ac953201dda8b040584e8b3c395e5aa1da4133fa))
* **companion:** persist the companion and offer a new one ([c99fd18](https://github.com/antwika/antwika/commit/c99fd18725a35c24d0b92f00bdef9c4999d140b9))
* **companion:** word the readout, the bubble and the button from a catalogue ([f07d024](https://github.com/antwika/antwika/commit/f07d0249ba3547c79ee70ace0b49cf34144ed3d3))
* **game:** a build palette down the right, a game menu on top, the readouts below ([dc1ee40](https://github.com/antwika/antwika/commit/dc1ee406df23ed8fffbff47c9b046c3093d4514d))
* **game:** a farm that feeds a house, through a store and a market ([af1441c](https://github.com/antwika/antwika/commit/af1441cba5defe69d105c90faebaba500534e007))
* **game:** a house that grows on what keeps reaching it ([bffdbf1](https://github.com/antwika/antwika/commit/bffdbf1d63ad8d9a7422ca9afcfff0c5ac73d003))
* **game:** a menu modal opened by F10 or the toolbar ([b35ea7a](https://github.com/antwika/antwika/commit/b35ea7aaa10cc2bb4458f4f2cd701ccac4d8c5b1))
* **game:** key bindings the recording carries, and the screen to change them ([fe50306](https://github.com/antwika/antwika/commit/fe50306419e739a8678d60fddb12a69c65717d91))
* **game:** lay a run of road by dragging the left button ([9b0bdfd](https://github.com/antwika/antwika/commit/9b0bdfd43d280c0251b97282803b42dff4b73c49))
* **game:** pause only when a player asks, and never as a side effect ([6b715d1](https://github.com/antwika/antwika/commit/6b715d1f6abd25c2e404f1b479415617cebc4520))
* **game:** people to house, jobs to fill and a bar that judges the city ([38adafb](https://github.com/antwika/antwika/commit/38adafb2b5f71b28810b3a010eecbacdb920c171))
* **game:** publish the coverage seam before anything writes to it ([5a429b5](https://github.com/antwika/antwika/commit/5a429b5ec70573c83cde83f095906da7211cd98c))
* **game:** say how full a hovered house is ([2bc97b9](https://github.com/antwika/antwika/commit/2bc97b9f2991d1dd8bd15bcaab5c95e28f38a2be))
* **game:** serve a district rather than hand it a number ([835b066](https://github.com/antwika/antwika/commit/835b066bb8767c62bcf587eae9a6b166fd352cc2))
* **game:** the round-one vocabulary, and every crossing as a table ([5603e1f](https://github.com/antwika/antwika/commit/5603e1f84a4039dead6f5ca18ccef9a4772caa11))
* **game:** the view scales with the window, and F10 fills the screen ([21f51e9](https://github.com/antwika/antwika/commit/21f51e909b1fa31a9688d11d14dc748b7a1b9467))
* **game:** three pivot-anchored texture atlases replace the one sheet ([3bdeceb](https://github.com/antwika/antwika/commit/3bdeceb8192afd3315bbee8f7bbf6fb8f34364d6))
* **gfx:** a window that can fill the screen, on resizable's terms ([f6326ad](https://github.com/antwika/antwika/commit/f6326ad12e0c088d17d17a82fb0c2bb456e21ff1))
* **gfx:** draw and measure text from a font::GlyphAtlas ([15ec77e](https://github.com/antwika/antwika/commit/15ec77e09b28a2b52a631743d5ee1ddb396b1443))
* **gfx:** draw the built-in font from Roboto Mono, not a table of bits ([4c48b4a](https://github.com/antwika/antwika/commit/4c48b4a1ed3d238925e155c59c510a813cb0587f))
* **gfx:** place a fixed canvas in a window of any size, scaled ([b347807](https://github.com/antwika/antwika/commit/b34780730a7a3ae0956472902dcd0ca2a3928d99))
* **i18n:** say how an application reaches a translator, and add its ids ([d9db10f](https://github.com/antwika/antwika/commit/d9db10f1b1d4afe33206a852aa08ee594b65d15d))
* **input:** a key is where it is, not what it types ([ddef8e3](https://github.com/antwika/antwika/commit/ddef8e3f386c8ff525c5f459293be9b423f993cc))
* **input:** read a device position as a position on the app's surface ([25ec65e](https://github.com/antwika/antwika/commit/25ec65e2da122010984496a25a490c846f6df7d6))
* **music_editor,input:** paste from the system clipboard ([02b150b](https://github.com/antwika/antwika/commit/02b150b98d8f4891ac0b4fae084b1a418f6aa6e1))
* **music_editor,sequencer:** a speed box that retimes every voice ([49d6e08](https://github.com/antwika/antwika/commit/49d6e08ef75e24c35c640f238a29c2399a72c4d9))
* **music_editor,ui,notation:** light the notes that are sounding ([13e6ad7](https://github.com/antwika/antwika/commit/13e6ad7802165f5a935868fd6e4aefa8ea9199c9))
* **music_editor:** a menu -- new, save, load and quit ([7ba0388](https://github.com/antwika/antwika/commit/7ba038869a6417b89ba32521de25b1f518143693))
* **music_editor:** a swedish board, a clipboard, a bar and chains down the page ([9046f9b](https://github.com/antwika/antwika/commit/9046f9b1d4cbd12a3ce7f25db754d7024c21e5a7))
* **music_editor:** a voice is a line, composed by chaining calls ([c93c142](https://github.com/antwika/antwika/commit/c93c142a4e81c167b925171988401ccd7c6c8fdd))
* **music_editor:** F10 scales the picture instead of parking it ([fa0ca18](https://github.com/antwika/antwika/commit/fa0ca18320c5713bfdef5aa19a65450b43347367))
* **music_editor:** four lines of notation you type into while they play ([557d4a7](https://github.com/antwika/antwika/commit/557d4a74da638673d8a49231dce1b756a22db01c))
* **music_editor:** one pane of code, at twice the glyph size ([2edf54e](https://github.com/antwika/antwika/commit/2edf54eaecfee0c8af29bb92279a93469afed497))
* **music_editor:** run until closed rather than to a tick budget ([d959165](https://github.com/antwika/antwika/commit/d959165bc74a63611df6435f232f17e086348f54))
* **network:** a sockets backend, and three contracts it corrected ([608ec37](https://github.com/antwika/antwika/commit/608ec37a46f6d893a96601b276ecd0a2cd3578c1))
* **network:** a transport seam that owns no thread, and a loopback one ([e84ae78](https://github.com/antwika/antwika/commit/e84ae787b93f950c78af5cbf27d378db272f4a09))
* **pattern:** a pattern algebra over exact rational cycles ([a88a717](https://github.com/antwika/antwika/commit/a88a717f46692b819a217b58f24c526e53300873))
* **poker:** draw the table as a ring of seats around a rounded felt ([98cfd0b](https://github.com/antwika/antwika/commit/98cfd0bb24e4708c2a7c08b6b1c9f88a05b43d1f))
* **sequencer,notation:** musical time meets frame time, and a notation ([73e807e](https://github.com/antwika/antwika/commit/73e807ee88b0aa05e9577c626342b44e266681f6))
* **sudoku:** a Sudoku you play, and a solver bounded by a tick ([34dd8a3](https://github.com/antwika/antwika/commit/34dd8a3e9ad1e765617d5052b5286b1933f367a2))
* **sudoku:** print through the catalogue, and take a locale on the line ([59b085e](https://github.com/antwika/antwika/commit/59b085e3d90d1672ec4c4e140fb256e8f5cb5f07))
* **synth:** sounds as values in source rather than waveform files ([d6daf73](https://github.com/antwika/antwika/commit/d6daf73f1465c094334a2b6ddd51d39eae7b035f))
* **task_worker:** a window that shows the queue being pulled in order ([d1e979e](https://github.com/antwika/antwika/commit/d1e979e07074645d140400f7c8796e9bdbc1cecd))
* **tower_defence:** a campaign of levels, four kinds of mob, waves and a kept high score ([ffdc3a2](https://github.com/antwika/antwika/commit/ffdc3a2d7d6cb21969c427e7670b3f87b31930a7))
* **tween:** easing curves that stay exact ([b0a497b](https://github.com/antwika/antwika/commit/b0a497bff14dbadaf697d8a4d1abca36e9d3ad6b))
* **ui_demo:** a textArea page, so the showcase shows every element (FINDINGS-3 32) ([33d00ae](https://github.com/antwika/antwika/commit/33d00ae3d42376b4314558909c7633d3c39ea477))
* **ui_demo:** word the whole showcase from a catalogue ([07eb045](https://github.com/antwika/antwika/commit/07eb0452f7e1b374c86a3a59517dc7c4f2c33fd7))
* **ui,music_editor:** Home and End move the caret over its line ([664766a](https://github.com/antwika/antwika/commit/664766a143379b753701db9f56487202e42a2b17))
* **ui:** a text area that selects, scrolls and takes a click ([768967b](https://github.com/antwika/antwika/commit/768967b866f3c2650088a753e7516163bd30bab5))
* **ui:** a text area, and a character that keeps its place in the order ([a771636](https://github.com/antwika/antwika/commit/a77163652962b737e38fe72f42a928cfa20c22d8))


### BREAKING CHANGES

* `ReplayWriter::Layout` is gone, since a record has to
fit on one line; `replayToJson()` is replaced by `replayHeaderToJson()`
and `replayRecordsFromJson()`; `saveReplayFile()` takes its events by
const reference and no layout. A build older than this refuses a JSON
Lines replay by version rather than by shape.

Co-Authored-By: Claude Opus 5 (1M context) <noreply@anthropic.com>
* antwika::i18n::MessageId no longer carries any
application's ids, and Catalogue, CatalogueEntry, lookup(), format()
and Translator are now templates.  A consumer declares its own id
enumeration and a Messages type satisfying antwika::i18n::MessageSet.

# [0.11.0](https://github.com/antwika/antwika/compare/v0.10.0...v0.11.0) (2026-08-01)


### Bug Fixes

* **app:** [M5] parse a command line once, flags of the app included ([9bd9018](https://github.com/antwika/antwika/commit/9bd9018cbebbfdf600efb35b8d79112bc176405e))
* **app:** [M5] report a refused command line instead of terminating ([33a2efd](https://github.com/antwika/antwika/commit/33a2efd9a66536c88c7e562d03d80b6b3230df74))
* **app:** mark runGuarded's catch the way every other one in src/ is ([0da4bd7](https://github.com/antwika/antwika/commit/0da4bd73c5853a4ed95104f103f8e03aee3ae191))
* **app:** report a recording that could not be saved ([57ac638](https://github.com/antwika/antwika/commit/57ac6384ee7bae3f22467cce56a1d849ff273a74))
* **apps:** [H3] poker and gfx_demo catch what their siblings catch ([d639b17](https://github.com/antwika/antwika/commit/d639b179d6ca8ec65943e36135d9527aa72822a1))
* **build:** a MinGW build that configures, and stops where it can't run ([1e8ed2b](https://github.com/antwika/antwika/commit/1e8ed2b480e28263f4695f68e3d7bea8513dc1e5))
* **cmake:** let a test target decline the shared precompiled header ([8e3651c](https://github.com/antwika/antwika/commit/8e3651c1179ff499289397342b184d808bef7997))
* **coverage:** the two lines gcov attributes to a path nothing takes ([80d8ee9](https://github.com/antwika/antwika/commit/80d8ee94dd5cc5910795d3858473a8e810333ad1))
* **ecs:** [M8] leave the world whole when a commit throws ([2fd3729](https://github.com/antwika/antwika/commit/2fd3729293284ab8f921cf550839ef96f77f2422))
* **game:** [H7] let the generator read the atlas layout ([88ed41a](https://github.com/antwika/antwika/commit/88ed41a74ac4209b36eaf6453635fd529d18cd10))
* **game:** [H8] one canvas the app and its test both read ([7919f48](https://github.com/antwika/antwika/commit/7919f48e390b10e2f861b351ab87276a03e1f2d9))
* **game:** [L13] no overlay now means no UiSink ([0fb5054](https://github.com/antwika/antwika/commit/0fb5054b4f5622ed8c816a681627851a872f4617))
* **game:** [M18] make a wrong atlas layout a build error ([979afac](https://github.com/antwika/antwika/commit/979afac4ed0e7246382382bdc3ef5a3e3c011c02))
* **game:** build nothing on the path that has nothing to report ([f5c2009](https://github.com/antwika/antwika/commit/f5c20093c47d38da0d5dad04b7bdd735d6c5e7f7))
* **game:** close the coverage holes the save format's schema opened ([3ba388c](https://github.com/antwika/antwika/commit/3ba388c6c4210774c73f740a5ad5adb864d3fc91))
* **game:** give every city its own buildings and walkers ([d0f4f83](https://github.com/antwika/antwika/commit/d0f4f83502bf0f28dc826e84ea17cc9a8b25060f))
* **gfx_demo:** keep the badge on a canvas taller than it is wide ([f653159](https://github.com/antwika/antwika/commit/f6531592a4333bf5beac2313388c862a34455b1e))
* **gfx:** sample a scaled texture the same way under both backends ([64e8cd9](https://github.com/antwika/antwika/commit/64e8cd9df09769f6d9d8f2893c196916f2afae8e))
* **holdem:** [H1] offer the action to a lone player who still owes ([7b9196d](https://github.com/antwika/antwika/commit/7b9196d2d56874573dd066fd4b66b8034f22d0d9))
* **holdem:** [H2] keep a folded player's stake in the pot until it pays ([9178b17](https://github.com/antwika/antwika/commit/9178b171af252bffea937137a4c41cdd362de613))
* **holdem:** [H3] refuse a small blind bigger than the big blind ([2573f23](https://github.com/antwika/antwika/commit/2573f23a61b6baf246ed3f35949653f60f170152))
* **holdem:** [H4] leave a seat owing nothing once the hand is paid out ([7fa0bbe](https://github.com/antwika/antwika/commit/7fa0bbe20cc582b5f3d6b199685698923bff6d03))
* **holdem:** [M4] let gcov count what the showdown builds ([ff983a0](https://github.com/antwika/antwika/commit/ff983a067db0c46eab06ac90d0fb039856d7a8ff))
* **input:** [M10] refuse to name a key the reader would reject ([48494ca](https://github.com/antwika/antwika/commit/48494ca3e958ef47957011a3f17ed0daf9f59e0b))
* **life:** [M13] return whole observer lists so nothing is left unwound ([7cb9132](https://github.com/antwika/antwika/commit/7cb9132860916093b67908f7be939e89ea3cbcb2))
* **life:** refuse a toggle outside the board rather than let it escape ([2892155](https://github.com/antwika/antwika/commit/28921554ee1b9f8bcfd869c91e8752883e154d15))
* **merge:** drop a conflict marker left in the render system test ([15df820](https://github.com/antwika/antwika/commit/15df82023d689c3fec35c370a391e2ffbdbd8b20))
* **poker:** [M12] give the seat box padding a statement of its own ([c8668ec](https://github.com/antwika/antwika/commit/c8668ec1664b08b2f8525e79ed3c9161cffcb764))
* **poker:** draw the table against the size the window was asked for ([7f29455](https://github.com/antwika/antwika/commit/7f29455896fba720d5c6666303500a775b54d642))
* **poker:** one layout, and the table's art placed from it ([449e6c6](https://github.com/antwika/antwika/commit/449e6c6a764949c633515ab8fbfbb5b6d6469899))
* **replay,apps:** [H2] say which file could not be opened ([3e3736c](https://github.com/antwika/antwika/commit/3e3736cfbaa791af3078f5406755115dbdac8837))
* **replay:** [M5] two coverage holes the new CLI parser opened ([36c4d5c](https://github.com/antwika/antwika/commit/36c4d5c649790e957036b0ceb0c151b50054401a))
* **scheduler:** [M14] finish a job that throws, and let a finished one go ([a967eed](https://github.com/antwika/antwika/commit/a967eedfbf74126adb7f899d0bf70df4e596bbcd))
* **scripts:** refuse a slot the atlas cannot hold ([78b8d77](https://github.com/antwika/antwika/commit/78b8d77a3c565e36713d22e5294b7a2efa9e7eb3))
* **sdl3:** blend a translucent colour instead of overwriting it ([01d0a2b](https://github.com/antwika/antwika/commit/01d0a2b15a1b047e119a7f7abe199003ebdbbe27))
* **sound_demo:** a constant the lambda can reach without capturing it ([7ed278c](https://github.com/antwika/antwika/commit/7ed278cc5ff8f3eb1f992548ad27efb0c8b2439e))
* **tower_defence:** make the reserved strip the bar's own height ([b8ba05c](https://github.com/antwika/antwika/commit/b8ba05cca5f0a509a39f45765e6e8d9cc7e81fd1))
* **tower_defence:** spawn no mob a config leaves nowhere to stand ([c2fc79c](https://github.com/antwika/antwika/commit/c2fc79c0384df331b20ec3714f425f18d42b44ec))
* **ui:** only let the pointer move focus once focus is in play ([911994d](https://github.com/antwika/antwika/commit/911994d86234fa0790cfc2838870321890348bc6))
* **wfc:** [M6] refuse to read a value out of a non-singleton domain ([5c0cb59](https://github.com/antwika/antwika/commit/5c0cb593426c7cbc7c480df59b344d151ac35d96))
* **wfc:** [M7] compare domains by their bits alone ([bd82fe4](https://github.com/antwika/antwika/commit/bd82fe4f2e1960e840e36f22913b0c175ee89df9))
* **wfc:** [M9] stop the search order resting on how libm rounds a log ([1215a95](https://github.com/antwika/antwika/commit/1215a95fce247d395956b8a619a642c65125c5db))


### Features

* **animation:** a clip library that keeps no time of its own ([0e05a03](https://github.com/antwika/antwika/commit/0e05a03b95de8d7147a70c1d668666a85825ef37))
* **app:** [M1] one place for what every main.cpp repeated ([a7a4160](https://github.com/antwika/antwika/commit/a7a41607dcb02236292293f98868b9fb730b8554))
* **app:** answer --help instead of running the session ([93695be](https://github.com/antwika/antwika/commit/93695be3c1b3c489d50cc92f3fc0094478ca8e63))
* **atlas_editor:** a pixel editor for the sheet the game blits ([9a9c062](https://github.com/antwika/antwika/commit/9a9c0624de293b65d09c672113528eef5864575a))
* **build:** an application, everything it opens, in one directory ([ad054be](https://github.com/antwika/antwika/commit/ad054be8bbf4b01b9ff2566e494d32187007ffe6))
* **build:** one build folder, and a sound backend you can pick ([d7b36fd](https://github.com/antwika/antwika/commit/d7b36fdf1e45fd95a41636950cea857290b6c0af))
* **companion:** a meal it did not want is the third violation ([d13e822](https://github.com/antwika/antwika/commit/d13e8229eaa93838a888deeaa05ca891048f8060))
* **companion:** a tamagotchi with two needs and an end it can reach ([5b239d3](https://github.com/antwika/antwika/commit/5b239d3cef77a1a1364aad8d7e0b71b62fbfbc17))
* **ecs_commons:** a commons library for shared ECS content ([755ce29](https://github.com/antwika/antwika/commit/755ce299144b0dd44346a972c3a2dec24f3b983f))
* **game:** [M19] register the input fold ahead of the sinks that read it ([18532b9](https://github.com/antwika/antwika/commit/18532b97482ef7fe0b20ff11303e95efc4c092aa))
* **game:** a bordered placement block, cancelled by a right click ([9dafa79](https://github.com/antwika/antwika/commit/9dafa79c2a74ff4defc8288c1fa950a81b9c9c41))
* **game:** a build palette, and a ghost of what it would place ([506a45f](https://github.com/antwika/antwika/commit/506a45f4081b48c6c86e590ab2ad8761becc5f3d))
* **game:** a cancelled build reaches nothing selected rather than the road ([ad908dc](https://github.com/antwika/antwika/commit/ad908dcbd5a415df4f54f556752afffcbae491aa))
* **game:** a pause button on the toolbar, and a tick and fps readout ([4159c1a](https://github.com/antwika/antwika/commit/4159c1affe9b8bb1d3d8ff9aabfb13ddb54a6c4f))
* **game:** a resource economy, and one walker per building ([cea62b1](https://github.com/antwika/antwika/commit/cea62b1935d4d750fe37a764b3b521e70af7d277))
* **game:** a save/load screen built from the new field and dropdown ([879fb09](https://github.com/antwika/antwika/commit/879fb0976d8ec9018f9815b8e9d6600f21d26c46))
* **game:** a toolbar the grid does not fight over ([1e9f1f0](https://github.com/antwika/antwika/commit/1e9f1f026655c5a816bde41c399611bb5f154169))
* **game:** a WFC world map with four cities you can open ([f90f10b](https://github.com/antwika/antwika/commit/f90f10b9c9d809f3688e83851224ef69f557a79b))
* **game:** buildings that cover a square block of cells ([7e89d53](https://github.com/antwika/antwika/commit/7e89d53f993f34fa5e21c03110392ac2d21d7f17))
* **game:** draw the frames between two ticks, and slide the walkers ([7ac457c](https://github.com/antwika/antwika/commit/7ac457c1801d2869aac09e8d54c0ae29d9ca448b))
* **game:** enter a city paused, and say so when there is no frame rate yet ([f28a29c](https://github.com/antwika/antwika/commit/f28a29c691ed881239edfdf630021731b624dbb3))
* **game:** gauge what a building needs and a walker carries ([f7ad555](https://github.com/antwika/antwika/commit/f7ad55589f90e0b65990d02fc0932f2ac18a1442))
* **game:** houses and shops send walkers out onto the roads ([4e36a26](https://github.com/antwika/antwika/commit/4e36a2603940c957c8a44f2b4f48886db9af3bd2))
* **game:** let the demo replay build a house and fill its road ([f26b8d4](https://github.com/antwika/antwika/commit/f26b8d489af145a2fb15f4f3d57e08073018dbaa))
* **game:** make the main menu an app mode, not a modal ([7ca5ac1](https://github.com/antwika/antwika/commit/7ca5ac1740c189deed043af665ce975233e53eea))
* **game:** reach the world map from the menu, and build in a city ([236c136](https://github.com/antwika/antwika/commit/236c136be02147a5eea1e68a09a9fff1316ca8d5))
* **game:** render the grid from a texture atlas ([18e7d38](https://github.com/antwika/antwika/commit/18e7d38d83248b6b826e51a1f80098170947348b))
* **game:** save and load a session as a versioned JSON document ([4fc4087](https://github.com/antwika/antwika/commit/4fc4087175f3d5bf39ae3a2c40e13c3327571a64))
* **game:** save format v2, carrying the buildings and their links ([2dab55b](https://github.com/antwika/antwika/commit/2dab55b9d50a683ea03165afb41a513ce4d9b75b))
* **game:** say what a run built, not only what it paved ([918ad85](https://github.com/antwika/antwika/commit/918ad859d5781498a02133a5251a52acb8eada63))
* **game:** the ghost follows a free pointer, off the record ([1920121](https://github.com/antwika/antwika/commit/1920121b55281bba3f400eb737b907a03c496d57))
* **game:** walk a cell every second tick ([5efb7ad](https://github.com/antwika/antwika/commit/5efb7adce60596808d994b031648b1c6a46b7a83))
* **gfx_demo:** let the pointer press the demo's buttons ([a25de7e](https://github.com/antwika/antwika/commit/a25de7edd757a6c188c32d7f8db02b9876116bc9))
* **gfx3d_demo:** show the 3D path off, and keep it covered ([7d63acd](https://github.com/antwika/antwika/commit/7d63acdf26fc69d6f822b9b91da14b8d5341aea9))
* **gfx:** a PNG writer, so what a bitmap holds can leave the process ([f44a5cf](https://github.com/antwika/antwika/commit/f44a5cf21cb543f65055aedbcf9579760a9c2133))
* **gfx:** add a 3D seam beside the 2D one, not instead of it ([c94aa1b](https://github.com/antwika/antwika/commit/c94aa1be24a4c7b79e39c14b9a80a1a36a7847f0))
* **gfx:** name the two window sizes, and pin the resizable contract ([3476d7a](https://github.com/antwika/antwika/commit/3476d7a88ddc727209ea13bb583cd2f6959248db))
* **i18n:** a message catalogue keyed by name, not by English ([a5f98c8](https://github.com/antwika/antwika/commit/a5f98c8caef7bc9ce02b88c030dc0d30369fd871))
* **input:** [M17] name the input pipeline instead of stacking it by hand ([5368a4d](https://github.com/antwika/antwika/commit/5368a4d8c00630a6b0228653b5000f619453d3b3))
* **input:** a pointer position that reaches a renderer and no recording ([a288a17](https://github.com/antwika/antwika/commit/a288a178cbded2656085dfe86f36a3f8ab101df2))
* **input:** hold back pointer motion nothing reads ([277c54b](https://github.com/antwika/antwika/commit/277c54bd833847eb4a4bb57e060b71c4e4a74f95))
* **pathfinding:** an A* that knows nothing about grids ([d4cd464](https://github.com/antwika/antwika/commit/d4cd464fd09c99b5192fc40e2dce223e789edbbc))
* **poker:** draw the table from the texture atlas ([15b01cc](https://github.com/antwika/antwika/commit/15b01cce7a9ad5e8a9f7a07a724da849b6c6afe0))
* **poker:** generate a texture atlas for the table art ([37dd8ba](https://github.com/antwika/antwika/commit/37dd8ba87f8a3deaee88adc2995a4c90244ee28a))
* **poker:** pace the table at one action per second by default ([6a6d340](https://github.com/antwika/antwika/commit/6a6d340318115a0df8b68f6a4f35273e5798634c))
* **raylib:** draw meshes through the 3D renderer ([8673d0e](https://github.com/antwika/antwika/commit/8673d0e29cbc2bb80a367dc991a2a0ef5bb3b0da))
* **replay:** [H8] record the canvas a replay was made against ([4d948e1](https://github.com/antwika/antwika/commit/4d948e152ae2413db8764ed54004307556dfd826))
* **replay:** [M5] one strict CLI parser, and a --help to go with it ([a246fec](https://github.com/antwika/antwika/commit/a246fec081898242aeb9ab82fd1ea3061565fd22))
* **replay:** migrate a document to the current schema before reading it ([bcc1536](https://github.com/antwika/antwika/commit/bcc1536aee8f8c0581800726362b34318266bf75))
* **replay:** version every persisted schema explicitly ([9dbf88e](https://github.com/antwika/antwika/commit/9dbf88e9bee56c2d759c2aa2050806cb8764ea88))
* **sound_demo:** a demo that proves the seam, and CI that runs it ([dbc2cd3](https://github.com/antwika/antwika/commit/dbc2cd310750caa10336830e6a14e6d2d0270e8c))
* **sound:** a PCM sound library that owns no thread and no clock ([ea33784](https://github.com/antwika/antwika/commit/ea33784f0e8d49cd911e0eeab11af61eb8b59bd7)), closes [hi#priority](https://github.com/hi/issues/priority)
* **sound:** an SDL3 device, pumped rather than driven ([5397e06](https://github.com/antwika/antwika/commit/5397e0632a0d9d6da9c0d08162c42f02fed5c4b0))
* **tower_defence:** a demo session to replay ([069a5dc](https://github.com/antwika/antwika/commit/069a5dc7f459380c63b58e9d5f9a3074bdedfc3f))
* **tower_defence:** generate a provably linear level with wfc ([fc082e8](https://github.com/antwika/antwika/commit/fc082e807f4f0862cea44eab0b2597e08fbeae67))
* **tower_defence:** mobs, towers and a score over the generated level ([c9cb4aa](https://github.com/antwika/antwika/commit/c9cb4aa5162e4a9326717ad2d337febba6f20504))
* **tower_defence:** run the battle as an app with a score bar ([4510690](https://github.com/antwika/antwika/commit/45106907ea595746c5f84391b5fa841280be5061))
* **ttf:** a TrueType library that draws glyphs and nothing else ([305ce5b](https://github.com/antwika/antwika/commit/305ce5b9905f749f1d90211286fff354c29833d4))
* **ui_demo:** an application that shows every antwika::ui element ([8bf0a3d](https://github.com/antwika/antwika/commit/8bf0a3de3eade7cbac6580fa094ef43592cba11f))
* **ui:** [M18] let a frame's widget ids be checked at compile time ([005d09a](https://github.com/antwika/antwika/commit/005d09a9163f0a8683c3ecfea7131a1e2b6ac08c))
* **ui:** a text field and a dropdown that hold nothing of their own ([dc14a0a](https://github.com/antwika/antwika/commit/dc14a0a85ef7e840399d8115848d5a20bf67a310))
* **ui:** keyboard focus, Tab navigation and a focus ring ([bef3d5e](https://github.com/antwika/antwika/commit/bef3d5e16a4ceddfb1763d7309a8b907092dad6a))
* **ui:** report where each named widget was laid out ([a6d0a98](https://github.com/antwika/antwika/commit/a6d0a98837685dcc451ece0ab90fa74249c46cc7))
* **ui:** resolve a pointer against the layout it draws ([e56ed92](https://github.com/antwika/antwika/commit/e56ed92b4fa3059ed212a2e5af313dffbc3198b7))
* **ui:** resolve hover on the render side, from a pointer no recording holds ([e781c0d](https://github.com/antwika/antwika/commit/e781c0d0a742adea7db2959eec0cd55a9463954a))
* **wfc:** name the conversion every caller of Solver had to write ([68b0ec0](https://github.com/antwika/antwika/commit/68b0ec033048d7e71746ef17b82ae805b7dfc910))


### Performance Improvements

* [L8] read every .cpp once when hunting unused test doubles ([f77d2a8](https://github.com/antwika/antwika/commit/f77d2a8e663f2f48c4a01a355fbd359e0c34132f))
* **apps:** [H5] stop keeping every event nobody reads ([9c96058](https://github.com/antwika/antwika/commit/9c96058e6cf31dc2cb636e5f8ce49a0944f6a5f8))
* **ci:** stop paying for work no gate reads ([cb64fbe](https://github.com/antwika/antwika/commit/cb64fbecd5a0b4f92d32255266f00f6b8a5ba9a6))
* **event:** [L3] hand back the recording rather than a copy of it ([9da3956](https://github.com/antwika/antwika/commit/9da39566053ca918784466bd88f5592707902c27))
* **replay:** [M3] walk the recording once, not once per tick ([2a8a49e](https://github.com/antwika/antwika/commit/2a8a49e08978c5349959a47742c597e0231f33a6))
* **wfc:** [M7] make a domain's count a number it keeps, not a scan ([d55b284](https://github.com/antwika/antwika/commit/d55b284fdd8119b02d639b838bc3baecb4d8a251))

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
