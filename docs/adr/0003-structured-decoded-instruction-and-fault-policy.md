# 0003 Structured Decoded Instruction and Deterministic Fault Policy

We decouple instruction decoding from execution by introducing a pipeline-friendly `DecodedInstruction` struct. This intermediate representation serves as a unified seam shared by the ISA Interpreter, the upcoming Out-of-Order (OoO) engine (for dispatch into Reservation Stations and Reorder Buffer), and the JIT backend. Invalid operations, illegal instructions, and out-of-bound memory accesses throw strongly-typed `CpuFault` exceptions to guarantee deterministic error handling and testability.
