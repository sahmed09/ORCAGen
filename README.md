# ORCAGen

ORCAGen is a research-oriented framework for generating and evaluating **Proof-of-Concept (PoC) malware** and their corresponding **deception orchestration code**.

The repository contains deception playbooks, malware procedures and active defense knowledge bases, PoC, and orchestration code implementations using multiple Large Language Models (LLMs).

---

## Academic Disclaimer

This repository is intended **strictly for academic and research purposes** as part of an academic conference submission.

It demonstrates the generation and implementation of both:

* **Proof-of-Concept (PoC) malware**
* **Corresponding deception orchestration code**

The code is provided to support reproducible and controlled experimentation, and the evaluation of LLM-assisted cyber-deception techniques.

> **Do not deploy, execute, or use any part of this repository against systems, networks, applications, or users without explicit authorization or for malicious purposes.**

The authors do not endorse or support malicious, unauthorized, or illegal use of the provided source code.

---

## Repository Structure

```text
ORCAGen/
│
├── Deception_Playbook/
│   └── Contains the SupeDLL implementations used for deception against keyloggers, information stealers, and ransomware.
│
├── Knowledge_Base/
│   └── Contains the knowledge base describing malware procedures and corresponding active-defense techniques in JSON format.
│
└── PoC_and_Orchestration_Codes/
    └── Contains:
        ├── Proof-of-Concept (PoC) malware source code
        ├── Corresponding deception orchestration (DLL) code
        └── Injector source code
```

### Deception_Playbook

The `Deception_Playbook` directory contains the **SupeDLL** implementations used to perform deception against the malware behaviors considered in this project.

The implemented malware categories include:

* Keyloggers
* Information stealers
* Ransomware

### Knowledge_Base

The `Knowledge_Base` directory contains the structured knowledge base used by the framework.

* The Malware Procedures knowledge base captures the target behaviors of a malware family.
* The Active Defenses knowledge base maps those behaviors to deception strategies.

The knowledge base is stored primarily in **JSON format**.

### PoC_and_Orchestration_Codes

The `PoC_and_Orchestration_Codes` directory contains the **Proof-of-Concept (PoC) malware code**, corresponding **deception orchestration code**, and injectors code for the five evaluated LLMs:

* GPT-4o
* GPT-5.5
* Gemini 3.5 Flash
* Claude Sonnet 4.5
* Qwen3-Coder

---

# Build Instructions

## Prerequisites

The implementation is designed for Windows environments.

### Operating System

* Windows 7
* Windows 10

### Development Environment

* Visual Studio 2019 or Visual Studio 2022
* C++
* x86 or x64 build configuration

### EasyHook

This project uses **EasyHook** for user-space API interception and DLL injection.

EasyHook documentation: https://easyhook.github.io/

Depending on the selected architecture, the following EasyHook components are required:

```text
EasyHook32.dll or EasyHook64.dll

EasyHook32.lib or EasyHook64.lib

EasyHook.h
```

EasyHook can be installed through the Visual Studio NuGet Package Manager as described below.

---

# Full Step-by-Step Build Guide

The following example demonstrates how to construct a Visual Studio solution for a component such as `ClipboardLogger`.

## 1. Create a New Visual Studio Solution

Create a new Visual Studio solution.

Example:

```text
ClipboardLogger
```

The solution should contain three main projects:

```text
ClipboardLogger
│
├── ClipboardLoggerPOC
├── AntiClipboardLoggerHook
└── Injector
```

---

## 2. Create the PoC Project

Create a console application for the Proof-of-Concept (PoC) malware.

1. Open:

   ```text
   File > New > Project
   ```

2. Select:

   ```text
   Console App
   ```

3. Configure it as an **Empty Project**.

4. Disable precompiled headers if necessary.

5. Name the project:

   ```text
   ClipboardLoggerPOC
   ```

6. Select **C++** as the programming language.

7. Add the provided source file or copy the corresponding logic from:

   ```text
   ClipboardLoggerPOC.cpp
   ```

---

## 3. Create the DLL Project

Create the DLL containing the corresponding API-hooking and deception logic.

1. Right-click the solution.

2. Select:

   ```text
   Add > New Project
   ```

3. Create a C++/Win32 project.

4. Select **DLL** as the application type.

   > **Important:** The project must be configured as a Dynamic-Link Library (DLL).

5. Name the project:

   ```text
   AntiClipboardLoggerHook
   ```

6. Add the provided source file or copy the corresponding logic from:

   ```text
   AntiClipboardLoggerHook.cpp
   ```

---

## 4. Create the Injector Project

Create a separate console application responsible for injecting the defensive DLL.

1. Right-click the solution.

2. Select:

   ```text
   Add > New Project
   ```

3. Select:

   ```text
   Console App
   ```

4. Configure it as an **Empty Project**.

5. Name the project:

   ```text
   Injector
   ```

6. Add the provided source file or copy the corresponding logic from:

   ```text
   Injector.cpp
   ```

---

## 5. Install EasyHook Using NuGet

EasyHook must be configured for both the **DLL** and **Injector** projects.

For each project:

1. Right-click the project.

2. Select:

   ```text
   Manage NuGet Packages
   ```

3. Open the **Browse** tab.

4. Search for:

   ```text
   EasyHook
   ```

5. Install:

   ```text
   EasyHook.NativePackage
   EasyHook.NativePackage.Redist
   ```

Install both packages in:

* `AntiClipboardLoggerHook`
* `Injector`

The EasyHook package version used during development is:

```text
2.7.7097
```

---

## 6. Copy the EasyHook Runtime DLL

The appropriate EasyHook runtime DLL must be available in the same output directory as `Injector.exe`.

Use:

```text
EasyHook32.dll
```

for an **x86/Win32** build.

Use:

```text
EasyHook64.dll
```

for an **x64** build.

An example package location for the 32-bit DLL is:

```text
C:\Users\User\Desktop\ClipboardLogger\packages\EasyHookNativePackage.redist.2.7.7097\build\native\bin\Win32\v141\Debug\EasyHook32.dll
```

An example package location for the 64-bit DLL is:

```text
C:\Users\User\Desktop\ClipboardLogger\packages\EasyHookNativePackage.redist.2.7.7097\build\native\bin\x64\v141\Debug\EasyHook64.dll
```

Copy the appropriate DLL into the Injector build output directory.

For example:

```text
C:\Users\User\Desktop\ClipboardLogger\Debug
```

or:

```text
C:\Users\User\Desktop\ClipboardLogger\Release
```

For a 32-bit build, the final directory may look similar to:

```text
Debug/
│
├── AntiClipboardLoggerHook.dll
├── ClipboardLoggerPoC.exe
├── EasyHook32.dll
└── Injector.exe
```

For a 64-bit build:

```text
Debug/
│
├── AntiClipboardLoggerHook.dll
├── ClipboardLoggerPoC.exe
├── EasyHook64.dll
└── Injector.exe
```

The exact output structure may vary depending on the Visual Studio configuration.

---

## 7. Configure the Project Properties

The following configuration should be applied to the **DLL** and **Injector** projects.

### Additional Include Directories

Open:

```text
Project Properties
> Configuration Properties
> C/C++
> General
> Additional Include Directories
```

Add the EasyHook include directory.

Example:

```text
C:\Users\User\Desktop\ClipboardLogger\packages\EasyHookNativePackage.2.7.7097\build\native\include
```

---

### Additional Library Directories

Open:

```text
Project Properties
> Configuration Properties
> Linker
> General
> Additional Library Directories
```

Add the EasyHook library directory.

Example for Win32:

```text
C:\Users\User\Desktop\ClipboardLogger\packages\EasyHookNativePackage.2.7.7097\build\native\lib\Win32\v141\Debug
```

For an x64 build, select the corresponding x64 library directory.

---

### Additional Dependencies

Open:

```text
Project Properties
> Configuration Properties
> Linker
> Input
> Additional Dependencies
```

For an **x86/Win32** build, add:

```text
EasyHook32.lib
```

For an **x64** build, add:

```text
EasyHook64.lib
```

---

## 8. Verify the Build Architecture

All components should use a compatible target architecture.

For example, a 32-bit configuration should consistently use:

```text
PoC        -> Win32/x86
DLL        -> Win32/x86
Injector   -> Win32/x86
EasyHook   -> EasyHook32
```

A 64-bit configuration should consistently use:

```text
PoC        -> x64
DLL        -> x64
Injector   -> x64
EasyHook   -> EasyHook64
```

Mixing x86 and x64 components may cause DLL loading or injection failures.

---

## 9. Select the Build Configuration

Select either:

```text
Debug
```

or:

```text
Release
```

Ensure that the following settings correspond to the selected configuration:

* Include directories
* Library directories
* Linker dependencies
* Runtime DLLs
* Target architecture

For example, when compiling in `Debug` mode, verify that all project settings refer to the appropriate Debug configuration.

Likewise, when compiling in `Release` mode, configure the corresponding Release settings.

---

## 10. Build the Solution

Build the complete Visual Studio solution using:

```text
Ctrl + Shift + B
```

Alternatively, select:

```text
Build > Build Solution
```

Verify that the following components compile successfully:

```text
PoC executable
DLL
Injector executable
```

---

# Important Build Notes

EasyHook binaries, library directories, package versions, and Visual Studio toolset directories may differ depending on the local environment.

Therefore, paths such as:

```text
C:\Users\User\Desktop\ClipboardLogger\...
```

should be treated as **examples only**.

Update them according to the actual EasyHook package location on the target machine.

In particular, directories such as:

```text
v141
Win32
x64
Debug
Release
```

may differ depending on:

* Visual Studio version
* Platform toolset
* Target architecture
* EasyHook package version
* Build configuration

---

# Notes for Reviewers

This repository primarily provides the **main C++ source files and research artifacts** rather than complete Visual Studio solution files for every experiment.

Reviewers who wish to compile the implementations may therefore need to create the corresponding Visual Studio solution and project structure manually.

The build procedure above provides a reference configuration for reproducing the implementations.

---

# Research Scope

The provided PoCs are intended to demonstrate representative malware behaviors required for evaluating the proposed deception mechanisms.

The repository should therefore be interpreted as a **controlled research testbed**, not as production malware tooling.

The accompanying defensive components are designed to investigate how LLM-generated orchestration can be used to manipulate or disrupt selected malware behaviors within authorized experimental environments.

---

# Ethical and Legal Notice

> **This software is provided exclusively for security research, academic evaluation, and educational purposes.**

Users are responsible for ensuring that all experiments are performed:

* In isolated or controlled environments
* On systems they own or are explicitly authorized to test
* In accordance with applicable laws
* In accordance with institutional policies
* In accordance with ethical research guidelines

Unauthorized deployment, execution, modification, or use of this code against third-party systems is strictly prohibited.

The authors assume no responsibility for misuse of the software or for damages resulting from unauthorized or improper use.
