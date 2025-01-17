Developer Notes
===============

These notes will be updated for Pretty Woman Coin when the Pretty Woman Coin team begin
accepting code contributions.

Various coding styles have been used during the history of the codebase,
and the result is not very consistent. However, we're now trying to converge to
a single style, so please use it in new code. Old code will be converted
gradually and you are encouraged to use the provided
[clang-format-diff script](/contrib/devtools/README.md#clang-format-diffpy)
to clean up the patch automatically before submitting a pull request.

- Basic rules specified in [src/.clang-format](/src/.clang-format).
  - Braces on new lines for namespaces, classes, functions, methods.
  - Braces on the same line for everything else.
  - 4 space indentation (no tabs) for every block except namespaces.
  - No indentation for `public`/`protected`/`private` or for `namespace`.
  - No extra spaces inside parenthesis; don't do ( this )
  - No space after function names; one space after `if`, `for` and `while`.
  - Always add braces for block statements (e.g. `if`, `for`, `while`).
  - `++i` is preferred over `i++`.
  - Use CamelCase for functions/methods, and lowerCamelCase for variables.
    - GLOBAL_CONSTANTS should use UPPER_SNAKE_CASE.
    - namespaces should use lower_snake_case.
  - Function names should generally start with an English command-form verb 
    (e.g. `ValidateTransaction`, `AddTransactionToMempool`, `ConnectBlock`)
  - Variable names should generally be nouns or past/future tense verbs.
    (e.g. `canDoThing`, `signatureOperations`, `didThing`)
  - Avoid using globals, remove existing globals whenever possible.
  - Class member variable names should be prepended with `m_`
  - DO choose easily readable identifier names. 
  - DO favor readability over brevity.
  - DO NOT use Hungarian notation.
  - DO NOT use abbreviations or contractions within identifiers. 
    - WRONG: mempool
    - RIGHT: MemoryPool
    - WRONG: ChangeDir
    - RIGHT: ChangeDirectory
  - DO NOT use obscure acronyms, DO uppercase any acronyms.
  - FINALLY, do not migrate existing code unless refactoring. It makes
    forwarding-porting from Prettywomancoin Core more difficult.

The naming convention roughly mirrors [Microsoft Naming Conventions](https://docs.microsoft.com/en-us/dotnet/standard/design-guidelines/general-naming-conventions)

C++ Coding Standards should strive to follow the [LLVM Coding Standards](https://llvm.org/docs/CodingStandards.html)

Code style example:
```c++
// namespaces should be lower_snake_case
namespace foo_bar_bob {

/**
 * Class is used for doing classy things.  All classes should
 * have a doxygen comment describing their PURPOSE.  That is to say,
 * why they exist.  Functional details can be determined from the code.
 * @see PerformTask()
 */
class Class {
private:
    //! memberVariable's name should be lowerCamelCase, and be a noun.
    int m_memberVariable;

public:
    /**
    * The documentation before a function or class method should follow Doxygen
    * spec. The name of the function should start with an english verb which 
    * indicates the intended purpose of this code.
    * 
    * The  function name should be should be CamelCase.
    * 
    * @param[in] s    A description
    * @param[in] n    Another argument description
    * @pre Precondition for function...
    */
    bool PerformTask(const std::string& s, int n) {
        // Use lowerChamelCase for local variables.
        bool didMore = false;

        // Comment summarizing the intended purpose of this section of code
        for (int i = 0; i < n; ++i) {
            if (!DidSomethingFail()) {
              return false;
            }
            ...
            if (IsSomethingElse()) {
                DoMore();
                didMore = true;
            } else {
                DoLess();
            }
        }

        return didMore;
    }
}
}
```


Doxygen comments
-----------------

To facilitate the generation of documentation, use doxygen-compatible comment blocks for functions, methods and fields.

For example, to describe a function use:
```c++
/**
 * ... text ...
 * @param[in] arg1    A description
 * @param[in] arg2    Another argument description
 * @pre Precondition for function...
 */
bool function(int arg1, const char *arg2)
```
A complete list of `@xxx` commands can be found at http://www.stack.nl/~dimitri/doxygen/manual/commands.html.
As Doxygen recognizes the comments by the delimiters (`/**` and `*/` in this case), you don't
*need* to provide any commands for a comment to be valid; just a description text is fine.

To describe a class use the same construct above the class definition:
```c++
/**
 * Alerts are for notifying old versions if they become too obsolete and
 * need to upgrade. The message is displayed in the status bar.
 * @see GetWarnings()
 */
class CAlert
{
```

To describe a member or variable use:
```c++
int var; //!< Detailed description after the member
```

or
```cpp
//! Description before the member
int var;
```

Also OK:
```c++
///
/// ... text ...
///
bool function2(int arg1, const char *arg2)
```

Not OK (used plenty in the current source, but not picked up):
```c++
//
// ... text ...
//
```

A full list of comment syntaxes picked up by doxygen can be found at http://www.stack.nl/~dimitri/doxygen/manual/docblocks.html,
but if possible use one of the above styles.

To build doxygen locally to test changes to the Doxyfile or visualize your comments before landing changes:
```
# at the project root, call:
doxygen doc/Doxyfile
# output goes to doc/doxygen/html/
```

Development tips and tricks
---------------------------

**compiling for debugging**

- Autotools
Run configure with the --enable-debug option (this will also enable -DDEBUG_LOCKORDER), then make. Or run configure with
CXXFLAGS="-g -ggdb -O0" or whatever debug flags you need.

- CMake
To enable debug in CMake run `cmake -DCMAKE_BUILD_TYPE=Debug`. Add the `--enable-debug` option to enable -DDEBUG_LOCKORDER. Then run make.

**prettywomancoind.log**

If the code is behaving strangely, take a look in the prettywomancoind.log file in the data directory;
error and debugging messages are written there.

The -debug=... command-line option controls debugging; running with just -debug or -debug=1 will turn
on all categories (and give you a very large prettywomancoind.log file).

**running and debugging tests**

Unit tests are run via `make check`
For running functional tests, see `/test/README.md`

Simple example of debugging unit tests with GDB on Linux:
```
cd /build/src/test
gdb test_prettywomancoin
break interpreter.cpp:295  # No path is necessary, just the file name and line number
run
# GDB hits the breakpoint
p/x opcode  # print the value of the variable (in this case, opcode) in hex
c           # continue
```

Simple example of debugging unit tests with LLDB (OSX or Linux):
```
cd /build/src/test
lldb -- test_prettywomancoin
break set --file interpreter.cpp --line 295
run
```

**code instrumentation**

CMake and autotools builds support code sanitizers for gcc and clang.
In addition CMake buid also supports clang static code analysis.

For CMake they can be enabled through CMake gui or CMake curses gui where they
are listed and can be toggled on or off or by providing them on command line
with `-D` CMake list options:
```
cmake -D enable_asan=ON <path_to_source_code>
```

For autotools they can be enabled at configuration step:
```
../configure --enable-asan
```


Supported sanitizers are:

- `enable_asan` in CMake (`--enable-asan` for autotools) for detecting memory corruption, leaks, illegal memory access
  - This sanitizer does not work in combination with assembly version of crypto implementation
    so assembly must be disabled by setting `CRYPTO_USE_ASM` CMake list option to `OFF`
    (`cmake -D BITCOIN_DEV_USE_ADDRESS_SANITIZER=ON -D CRYPTO_USE_ASM=OFF <path_to_source_code>`)
  - This sanitizer does not work in combination with thread sanitizer being enabled
  - gcc sanitizer can trigger `LeakSanitizer does not work under ptrace` error. In that case run the executable
    with `ASAN_OPTIONS` environment variable (`ASAN_OPTIONS=detect_leaks=0 <executable_name>`) to disable the offending
    checks
- `enable_tsan` in CMake (`--enable-tsan` for autotools) for detecting race conditions (on gcc also potential dead locks)
  - This sanitizer does not work in combination with address or ub sanitizer being enabled
  - When sanitizer is enabled code execution is slowed down to approximately between 5 to 15 times
  - on gcc this sanitizer also does some static potential dead lock checking which can generate false positives
    as it by design doesn't consider execution order/order of data initialization
- `enable_ubsan` in CMake (`--enable-ubsan` for autotools) for detecting use of behavior that is undefined by C++ standard
  - This sanitizer does not work in combination with thread sanitizer being enabled
  - gcc version of the sanitizer doesn't print stacktraces by default so they need to be enabled by running executable
    with `UBSAN_OPTIONS` environment variable (`UBSAN_OPTIONS="print_stacktrace=1" <executable_name>`)
- `enable_static_analyzer` for static analysis of code during compilation

NOTE: Sanitizers change generated assembly code of an executable so running a program with them enabled
does not guarantee that executable will work with them being disabled so during testing they should not
be enable all the time. Since enabling sanitizers during the build changes the resulting executable,
disabling them means recompiling the executable with sanitizers disabled in CMake/autotools build.

NOTE: Sanitizers are a runtime feature so they report errors/warnings only when they occur during execution.
This is especially true for thread sanitizer which can only detect race conditions if they actually occur
during execution so if a single run passes without errors/warning there is no guarantee that the consecutive
runs will as well so they are not a replacement for testing.

**writing script integration tests**

Script integration tests are built using `src/test/script_tests.cpp`:

1. Uncomment the line with `#define UPDATE_JSON_TESTS`
2. Add a new TestBuilder to the `script_build` test to cover your test case.
3. `make && ./src/test/test_prettywomancoin --run_test=script_tests`
4. Copy your newly generated test JSON from `<build-dir>/src/script_tests.json.gen` to `src/test/data/script_tests.json`.

Please commit your TestBuilder along with your generated test JSON and cleanup the uncommented #define before code review.

**testnet and regtest modes**

Run with the -testnet option to run with "play prettywomancoins" on the test network, if you
are testing multi-machine code that needs to operate across the internet.

If you are testing something that can run on one machine, run with the -regtest option.
In regression test mode, blocks can be created on-demand; see test/functional/ for tests
that run in -regtest mode.

**DEBUG_LOCKORDER**

Prettywomancoin Core is a multithreaded application, and deadlocks or other multithreading bugs
can be very difficult to track down. Compiling with -DDEBUG_LOCKORDER (configure
CXXFLAGS="-DDEBUG_LOCKORDER -g") inserts run-time checks to keep track of which locks
are held, and adds warnings to the prettywomancoind.log file if inconsistencies are detected.

Locking/mutex usage notes
-------------------------

The code is multi-threaded, and uses mutexes and the
LOCK/TRY_LOCK macros to protect data structures.

Deadlocks due to inconsistent lock ordering (thread 1 locks cs_main
and then cs_wallet, while thread 2 locks them in the opposite order:
result, deadlock as each waits for the other to release its lock) are
a problem. Compile with -DDEBUG_LOCKORDER to get lock order
inconsistencies reported in the prettywomancoind.log file.

Re-architecting the core code so there are better-defined interfaces
between the various components is a goal, with any necessary locking
done by the components (e.g. see the self-contained CKeyStore class
and its cs_KeyStore lock for example).

Threads
-------

- ThreadScriptCheck : Verifies block scripts.

- ThreadImport : Loads blocks from blk*.dat files or bootstrap.dat.

- StartNode : Starts other threads.

- ThreadDNSAddressSeed : Loads addresses of peers from the DNS.

- ThreadMapPort : Universal plug-and-play startup/shutdown

- ThreadSocketHandler : Sends/Receives data from peers on port 3840.

- ThreadOpenAddedConnections : Opens network connections to added nodes.

- ThreadOpenConnections : Initiates new connections to peers.

- ThreadMessageHandler : Higher-level message handling (sending and receiving).

- DumpAddresses : Dumps IP addresses of nodes to peers.dat.

- ThreadFlushWalletDB : Close the wallet.dat file if it hasn't been used in 500ms.

- ThreadRPCServer : Remote procedure call handler, listens on port 7908 for connections and services them.

- PrettywomancoinMiner : Generates prettywomancoins (if wallet is enabled).

- Shutdown : Does an orderly shutdown of everything.

Ignoring IDE/editor files
--------------------------

In closed-source environments in which everyone uses the same IDE it is common
to add temporary files it produces to the project-wide `.gitignore` file.

However, in open source software such as Prettywomancoin Core, where everyone uses
their own editors/IDE/tools, it is less common. Only you know what files your
editor produces and this may change from version to version. The canonical way
to do this is thus to create your local gitignore. Add this to `~/.gitconfig`:

```
[core]
        excludesfile = /home/.../.gitignore_global
```

(alternatively, type the command `git config --global core.excludesfile ~/.gitignore_global`
on a terminal)

Then put your favourite tool's temporary filenames in that file, e.g.
```
# NetBeans
nbproject/
```

Another option is to create a per-repository excludes file `.git/info/exclude`.
These are not committed but apply only to one repository.

If a set of tools is used by the build system or scripts the repository (for
example, lcov) it is perfectly acceptable to add its files to `.gitignore`
and commit them.

Development guidelines
============================

A few non-style-related recommendations for developers, as well as points to
pay attention to for reviewers of Prettywomancoin Core code.

Wallet
-------

- Make sure that no crashes happen with run-time option `-disablewallet`.

  - *Rationale*: In RPC code that conditionally uses the wallet (such as
    `validateaddress`) it is easy to forget that global pointer `pwalletMain`
    can be NULL. See `test/functional/disablewallet.py` for functional tests
    exercising the API with `-disablewallet`

- Include `db_cxx.h` (BerkeleyDB header) only when `ENABLE_WALLET` is set

  - *Rationale*: Otherwise compilation of the disable-wallet build will fail in environments without BerkeleyDB

General C++
-------------

- Assertions should not have side-effects

  - *Rationale*: Even though the source code is set to to refuse to compile
    with assertions disabled, having side-effects in assertions is unexpected and
    makes the code harder to understand

- If you use the `.h`, you must link the `.cpp`

  - *Rationale*: Include files define the interface for the code in implementation files. Including one but
      not linking the other is confusing. Please avoid that. Moving functions from
      the `.h` to the `.cpp` should not result in build errors

- Use the RAII (Resource Acquisition Is Initialization) paradigm where possible. For example by using
  `unique_ptr` for allocations in a function.

  - *Rationale*: This avoids memory and resource leaks, and ensures exception safety

C++ data structures
--------------------

- Never use the `std::map []` syntax when reading from a map, but instead use `.find()`

  - *Rationale*: `[]` does an insert (of the default element) if the item doesn't
    exist in the map yet. This has resulted in memory leaks in the past, as well as
    race conditions (expecting read-read behavior). Using `[]` is fine for *writing* to a map

- Do not compare an iterator from one data structure with an iterator of
  another data structure (even if of the same type)

  - *Rationale*: Behavior is undefined. In C++ parlor this means "may reformat
    the universe", in practice this has resulted in at least one hard-to-debug crash bug

- Watch out for out-of-bounds vector access. `&vch[vch.size()]` is illegal,
  including `&vch[0]` for an empty vector. Use `vch.data()` and `vch.data() +
  vch.size()` instead.

- Vector bounds checking is only enabled in debug mode. Do not rely on it

- Make sure that constructors initialize all fields. If this is skipped for a
  good reason (i.e., optimization on the critical path), add an explicit
  comment about this

  - *Rationale*: Ensure determinism by avoiding accidental use of uninitialized
    values. Also, static analyzers balk about this.

- Use explicitly signed or unsigned `char`s, or even better `uint8_t` and
  `int8_t`. Do not use bare `char` unless it is to pass to a third-party API.
  This type can be signed or unsigned depending on the architecture, which can
  lead to interoperability problems or dangerous conditions such as
  out-of-bounds array accesses

- Prefer explicit constructions over implicit ones that rely on 'magical' C++ behavior

  - *Rationale*: Easier to understand what is happening, thus easier to spot mistakes, even for those
  that are not language lawyers

Strings and formatting
------------------------

- Use `std::string`, avoid C string manipulation functions

  - *Rationale*: C++ string handling is marginally safer, less scope for
    buffer overflows and surprises with `\0` characters. Also some C string manipulations
    tend to act differently depending on platform, or even the user locale

- Use `ParseInt32`, `ParseInt64`, `ParseUInt32`, `ParseUInt64`, `ParseDouble` from `utilstrencodings.h` for number parsing

  - *Rationale*: These functions do overflow checking, and avoid pesky locale issues

Variable names
--------------

The shadowing warning (`-Wshadow`) is enabled by default. It prevents issues rising
from using a different variable with the same name.

Please name variables so that their names do not shadow variables defined in the source code.

Threads and synchronization
----------------------------

- Build and run tests with `-DDEBUG_LOCKORDER` to verify that no potential
  deadlocks are introduced. As of 0.12, this is defined by default when
  configuring with `--enable-debug`

- When using `LOCK`/`TRY_LOCK` be aware that the lock exists in the context of
  the current scope, so surround the statement and the code that needs the lock
  with braces

  OK:

```c++
{
    TRY_LOCK(cs_vNodes, lockNodes);
    ...
}
```

  Wrong:

```c++
TRY_LOCK(cs_vNodes, lockNodes);
{
    ...
}
```

Source code organization
--------------------------

- Implementation code should go into the `.cpp` file and not the `.h`, unless necessary due to template usage or
  when performance due to inlining is critical

  - *Rationale*: Shorter and simpler header files are easier to read, and reduce compile time

- Don't import anything into the global namespace (`using namespace ...`). Use
  fully specified types such as `std::string`.

  - *Rationale*: Avoids symbol conflicts

Subtrees
----------

Several parts of the repository are subtrees of software maintained elsewhere.

Some of these are maintained by active developers of Prettywomancoin Core, in which case changes should probably go
directly upstream without being PRed directly against the project.  They will be merged back in the next
subtree merge.

Others are external projects without a tight relationship with our project.  Changes to these should also
be sent upstream but bugfixes may also be prudent to PR against Prettywomancoin Core so that they can be integrated
quickly.  Cosmetic changes should be purely taken upstream.

There is a tool in contrib/devtools/git-subtree-check.sh to check a subtree directory for consistency with
its upstream repository.

Current subtrees include:

- src/leveldb
  - Upstream at https://github.com/google/leveldb ; Maintained by Google, but open important PRs to Core to avoid delay

- src/libsecp256k1
  - Upstream at https://github.com/prettywomancoin-core/secp256k1/ ; actively maintaned by Core contributors.

- src/crypto/ctaes
  - Upstream at https://github.com/prettywomancoin-core/ctaes ; actively maintained by Core contributors.

- src/univalue
  - Upstream at https://github.com/jgarzik/univalue ; report important PRs to Core to avoid delay.


Git and GitHub tips
---------------------

- Github is not typically the source of truth for pull requests.  See CONTRIBUTING.md for instructions
  on setting up your repo correctly.

- For resolving merge/rebase conflicts, it can be useful to enable diff3 style using
  `git config merge.conflictstyle diff3`. Instead of

        <<<
        yours
        ===
        theirs
        >>>

  you will see

        <<<
        yours
        |||
        original
        ===
        theirs
        >>>

  This may make it much clearer what caused the conflict. In this style, you can often just look
  at what changed between *original* and *theirs*, and mechanically apply that to *yours* (or the other way around).

- When reviewing patches which change indentation in C++ files, use `git diff -w` and `git show -w`. This makes
  the diff algorithm ignore whitespace changes. This feature is also available on github.com, by adding `?w=1`
  at the end of any URL which shows a diff.

- When reviewing patches that change symbol names in many places, use `git diff --word-diff`. This will instead
  of showing the patch as deleted/added *lines*, show deleted/added *words*.

- When reviewing patches that move code around, try using
  `git diff --patience commit~:old/file.cpp commit:new/file/name.cpp`, and ignoring everything except the
  moved body of code which should show up as neither `+` or `-` lines. In case it was not a pure move, this may
  even work when combined with the `-w` or `--word-diff` options described above.

- When looking at other's pull requests, it may make sense to add the following section to your `.git/config`
  file:

        [remote "upstream-pull"]
                fetch = +refs/pull/*:refs/remotes/upstream-pull/*
                url = git@github.com:prettywomancoin/prettywomancoin.git

  This will add an `upstream-pull` remote to your git repository, which can be fetched using `git fetch --all`
  or `git fetch upstream-pull`. Afterwards, you can use `upstream-pull/NUMBER/head` in arguments to `git show`,
  `git checkout` and anywhere a commit id would be acceptable to see the changes from pull request NUMBER.
