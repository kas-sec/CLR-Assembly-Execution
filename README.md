# CLR In-Memory Assembly Execution Proof of Concept

## Overview

This project demonstrates in-memory execution of .NET assemblies using the Common Language Runtime (CLR) hosting API. It showcases how .NET executables can be loaded and executed directly from memory without touching disk, a technique commonly used in both legitimate software and security research contexts.

## How It Works

### Architecture Components

The project consists of two main components:

**Builder** - Encrypts a .NET assembly and embeds it into a loader stub
**Loader** - Extracts, decrypts, and executes the embedded assembly in memory

### Technical Workflow

#### 1. Builder Process (payload.cpp)

The builder takes a .NET assembly and prepares it for in-memory execution:

- Reads the target .NET assembly (DLL or EXE) from disk
- Generates random AES-256 encryption key and IV using BCrypt
- Encrypts the assembly bytes using AES-CBC mode
- Embeds the encrypted payload into a loader stub executable
- Appends an overlay header containing encryption metadata

**PE Overlay Structure:**

The overlay is appended to the end of the PE file and contains:
- Encrypted assembly bytes
- OverlayHeader structure with magic signature, encryption key, IV, and payload size

PE overlays are extra data beyond the defined PE sections that Windows ignores during normal execution but can be read programmatically. This technique allows arbitrary data storage without affecting the executable's functionality.

#### 2. Loader Process (main.cpp + clr.cpp)

The loader extracts and executes the embedded assembly:

**Step 1: Overlay Extraction**
- Opens its own executable file using GetModuleFileName
- Seeks to end of file minus sizeof(OverlayHeader)
- Reads and validates the overlay header (checks magic signature 0x434C5241)
- Extracts encrypted assembly bytes from the calculated position

**Step 2: Decryption**
- Decrypts the assembly using AES-256-CBC with the embedded key/IV
- Validates the decrypted data (checks for PE "MZ" header)
- Securely wipes encrypted data from memory

**Step 3: CLR Initialization**
- Loads mscoree.dll (managed execution engine)
- Creates CLR MetaHost interface via CLRCreateInstance
- Obtains .NET Framework v4.0.30319 runtime
- Starts the CLR and retrieves the default AppDomain

**Step 4: In-Memory Execution**
- Creates a SAFEARRAY containing the assembly bytes
- Calls AppDomain.Load_3() to load the assembly from the byte array
- Retrieves the assembly's entry point using IAssembly::get_EntryPoint
- Invokes the entry point method via IMethodInfo::Invoke_3
- Handles both parameterless Main() and Main(string[] args) signatures

## CLR Assembly Execution Details

### How .NET Assemblies Are Stored in Memory

.NET assemblies are typically PE files containing:
- Standard PE headers (DOS header, NT headers, section table)
- .NET metadata (assembly manifest, type definitions, method signatures)
- Common Intermediate Language (CIL) bytecode
- Resources and embedded data

When loaded via Assembly.Load(byte[]), the CLR:
1. Validates the PE structure and .NET metadata
2. Maps the assembly into the AppDomain's memory space
3. Creates internal data structures for type resolution
4. Does NOT require file system access - operates entirely in memory
5. JIT compiles CIL to native code on-demand during execution

### CLR Hosting Interfaces

The project uses COM-based CLR hosting interfaces:

- **ICLRMetaHost** - Entry point for CLR hosting, enumerates available runtimes
- **ICLRRuntimeInfo** - Represents a specific .NET runtime version
- **ICorRuntimeHost** - Manages the CLR lifecycle and AppDomain creation
- **_AppDomain** - Provides assembly loading capabilities (Load_3 method)
- **_Assembly** - Represents a loaded assembly, provides entry point access
- **_MethodInfo** - Enables method invocation via reflection

These interfaces allow unmanaged C++ code to host the .NET runtime and execute managed assemblies without P/Invoke or reverse P/Invoke complications.

## Cryptographic Implementation

**Algorithm:** AES-256-CBC
**Key Size:** 32 bytes (256 bits)
**IV Size:** 16 bytes (128 bits)
**Padding:** PKCS7 (BCRYPT_BLOCK_PADDING)

The crypto.cpp module uses Windows Cryptography API: Next Generation (CNG) via bcrypt.lib for secure encryption/decryption operations. Random key generation uses BCryptGenRandom for cryptographically secure entropy.

## Build Instructions

### Prerequisites

- Windows 10/11 SDK
- LLVM/Clang (clang-cl)
- Visual Studio 2022 (any edition: Community, Professional, Enterprise, or BuildTools)
- Windows Resource Compiler (rc.exe)

### Building the Project

Use the provided PowerShell build script:

.\build.ps1

To clean and rebuild:

.\build.ps1 -Clean

The script will automatically:
- Detect clang-cl and rc.exe
- Find the correct Windows SDK and MSVC toolset versions
- Compile the loader stub (release and debug versions)
- Compile resources
- Build the final builder executable

Output will be in the `build\` directory.

### Using the Builder

After building, create a payload from a .NET assembly:

.\build\builder.exe -i path\to\assembly.exe -o output_name

For debug mode (shows verbose execution logs):

.\build\builder.exe -i path\to\assembly.exe -o output_name --debug

The output executable will be in `outputs\output_name.exe` and can be run standalone on any Windows system with .NET Framework 4.0+ installed.

## Build Configuration

The project supports two build modes:

**Release Mode:** Silent execution with no console output (WinMain entry point)
**Debug Mode:** Verbose logging showing each execution step (main entry point with AllocConsole)

Debug output includes HRESULT values, byte counts, hex dumps, and step-by-step CLR initialization progress.

## Security Implications

This is a proof-of-concept demonstrating CLR hosting techniques. The same methods are used by:
- Legitimate software for plugin systems and scripting
- Red team tools for post-exploitation
- Malware for defense evasion

Understanding these techniques is valuable for both offensive security research and defensive detection engineering.

## Detection Considerations

Defensive detection opportunities include:
- Monitoring non-.NET processes loading CLR DLLs (mscoree.dll, clr.dll, mscorlib.ni.dll)
- Analyzing short-lived processes with CLR loaded
- Detecting Assembly.Load(byte[]) calls from unmanaged code
- Scanning for PE overlay anomalies
- Monitoring BCrypt API usage in conjunction with file I/O operations

## Educational Purpose

This code is provided for educational and research purposes to understand:
- CLR hosting from unmanaged code
- In-memory .NET assembly execution
- PE file structure and overlay manipulation
- Windows cryptography APIs
- COM interface usage with .NET runtime

## References

- Microsoft CLR Hosting Documentation
- PE File Format Specification
- .NET Assembly Loading Mechanisms
- Windows CNG Cryptography

---

**Disclaimer:** This tool is for authorized security research and education only. Ensure you have proper authorization before using these techniques in any environment.
