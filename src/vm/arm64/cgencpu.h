//
// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//
//


#ifndef _TARGET_ARM64_
#error Should only include "cGenCpu.h" for ARM64 builds
#endif

#ifndef __cgencpu_h__
#define __cgencpu_h__

#define INSTRFMT_K64
#include <stublink.h>

#define USE_REDIRECT_FOR_GCSTRESS

EXTERN_C void getFPReturn(int fpSize, INT64 *pRetVal);
EXTERN_C void setFPReturn(int fpSize, INT64 retVal);


class ComCallMethodDesc;


#define COMMETHOD_PREPAD                        24   // # extra bytes to allocate in addition to sizeof(ComCallMethodDesc)
#ifdef FEATURE_COMINTEROP
#define COMMETHOD_CALL_PRESTUB_SIZE             24
#define COMMETHOD_CALL_PRESTUB_ADDRESS_OFFSET   16   // the offset of the call target address inside the prestub
#endif // FEATURE_COMINTEROP

#define STACK_ALIGN_SIZE                        16

#define JUMP_ALLOCATE_SIZE                      16  // # bytes to allocate for a jump instruction
#define BACK_TO_BACK_JUMP_ALLOCATE_SIZE         16  // # bytes to allocate for a back to back jump instruction

#define HAS_NDIRECT_IMPORT_PRECODE              1

#define USE_INDIRECT_CODEHEADER

#ifdef FEATURE_REMOTING
#define HAS_REMOTING_PRECODE                    1
#endif

//ARM64TODO: Enable it once we complete work on precode
//#define HAS_FIXUP_PRECODE                       1
//#define HAS_FIXUP_PRECODE_CHUNKS                1

// ThisPtrRetBufPrecode one is necessary for closed delegates over static methods with return buffer
#define HAS_THISPTR_RETBUF_PRECODE              1

//ARM64TODO: verify this
#define CODE_SIZE_ALIGN                         8
#define CACHE_LINE_SIZE                         32  // As per Intel Optimization Manual the cache line size is 32 bytes
#define LOG2SLOT                                LOG2_PTRSIZE

#define ENREGISTERED_RETURNTYPE_MAXSIZE         64  // bytes (maximum HFA size is 8 doubles)
#define ENREGISTERED_RETURNTYPE_INTEGER_MAXSIZE 8   // bytes
#define ENREGISTERED_PARAMTYPE_MAXSIZE          16  // bytes (max value type size that can be passed by value)

#define CALLDESCR_ARGREGS                       1   // CallDescrWorker has ArgumentRegister parameter
#define CALLDESCR_FPARGREGS                     1   // CallDescrWorker has FloatArgumentRegisters parameter

// Given a return address retrieved during stackwalk,
// this is the offset by which it should be decremented to arrive at the callsite.
#define STACKWALK_CONTROLPC_ADJUST_OFFSET 4

//=======================================================================
// IMPORTANT: This value is used to figure out how much to allocate
// for a fixed array of FieldMarshaler's. That means it must be at least
// as large as the largest FieldMarshaler subclass. This requirement
// is guarded by an assert.
//=======================================================================
//ARM64TODO: verify this
#define MAXFIELDMARSHALERSIZE               40

//**********************************************************************
// Parameter size
//**********************************************************************

typedef INT64 StackElemType;
#define STACK_ELEM_SIZE sizeof(StackElemType)

// !! This expression assumes STACK_ELEM_SIZE is a power of 2.
#define StackElemSize(parmSize) (((parmSize) + STACK_ELEM_SIZE - 1) & ~((ULONG)(STACK_ELEM_SIZE - 1)))

//**********************************************************************
// Frames
//**********************************************************************

//--------------------------------------------------------------------
// This represents the callee saved (non-volatile) registers saved as
// of a FramedMethodFrame.
//--------------------------------------------------------------------
typedef DPTR(struct CalleeSavedRegisters) PTR_CalleeSavedRegisters;
struct CalleeSavedRegisters {
    INT64 x29; // frame pointer
    INT64 x30; // link register
    INT64 x19, x20, x21, x22, x23, x24, x25, x26, x27, x28;
};

//--------------------------------------------------------------------
// This represents the arguments that are stored in volatile registers.
// This should not overlap the CalleeSavedRegisters since those are already
// saved separately and it would be wasteful to save the same register twice.
// If we do use a non-volatile register as an argument, then the ArgIterator
// will probably have to communicate this back to the PromoteCallerStack
// routine to avoid a double promotion.
//--------------------------------------------------------------------
typedef DPTR(struct ArgumentRegisters) PTR_ArgumentRegisters;
struct ArgumentRegisters {
    INT64 x[8]; // x0 ....x7
};
#define NUM_ARGUMENT_REGISTERS 8

#define ARGUMENTREGISTERS_SIZE sizeof(ArgumentRegisters)


//--------------------------------------------------------------------
// This represents the floating point argument registers which are saved
// as part of the NegInfo for a FramedMethodFrame. Note that these
// might not be saved by all stubs: typically only those that call into
// C++ helpers will need to preserve the values in these volatile
// registers.
//--------------------------------------------------------------------
typedef DPTR(struct FloatArgumentRegisters) PTR_FloatArgumentRegisters;
struct FloatArgumentRegisters {
    // armV8 supports 32 floating point registers. Each register is 128bits long.
    // It can be accessed as 128-bit value or 64-bit value(d0-d31) or as 32-bit value (s0-s31)
    // or as 16-bit value or as 8-bit values. C# only has two builtin floating datatypes float(32-bit) and 
    // double(64-bit). It does not have a quad-precision floating point.So therefore it does not make sense to
    // store full 128-bit values in Frame when the upper 64 bit will not contain any values.
    double  d[8];  // d0-d7
};


//**********************************************************************
// Exception handling
//**********************************************************************

inline PCODE GetIP(const T_CONTEXT * context) {
    LIMITED_METHOD_DAC_CONTRACT;
    return context->Pc;
}

inline void SetIP(T_CONTEXT *context, PCODE eip) {
    LIMITED_METHOD_DAC_CONTRACT;
    context->Pc = eip;
}

inline TADDR GetSP(const T_CONTEXT * context) {
    LIMITED_METHOD_DAC_CONTRACT;
    return TADDR(context->Sp);
}

inline PCODE GetLR(const T_CONTEXT * context) {
    LIMITED_METHOD_DAC_CONTRACT;
    return PCODE(context->Lr);
}

extern "C" LPVOID __stdcall GetCurrentSP();

inline void SetSP(T_CONTEXT *context, TADDR sp) {
    LIMITED_METHOD_DAC_CONTRACT;
    context->Sp = DWORD64(sp);
}

inline void SetFP(T_CONTEXT *context, TADDR fp) {
    LIMITED_METHOD_DAC_CONTRACT;
    context->Fp = DWORD64(fp);
}

inline TADDR GetFP(const T_CONTEXT * context)
{
    LIMITED_METHOD_DAC_CONTRACT;
    return (TADDR)(context->Fp);
}

#ifdef FEATURE_COMINTEROP
void emitCOMStubCall (ComCallMethodDesc *pCOMMethod, PCODE target);
#endif // FEATURE_COMINTEROP

inline BOOL ClrFlushInstructionCache(LPCVOID pCodeAddr, size_t sizeOfCode)
{
#ifdef CROSSGEN_COMPILE
    // The code won't be executed when we are cross-compiling so flush instruction cache is unnecessary
    return TRUE;
#else
    return FlushInstructionCache(GetCurrentProcess(), pCodeAddr, sizeOfCode);
#endif
}

//------------------------------------------------------------------------
inline void emitJump(UINT32* pCode, LPVOID target)
{
    LIMITED_METHOD_CONTRACT;

    // We require 8-byte alignment so the LDR instruction is aligned properly
    _ASSERTE(((UINT_PTR)pCode & 7) == 0);

    // +0:   ldr x16, [pc, #8]
    // +4:   br  x16
    // +8:   [target address]
    
    pCode[0] = 0x58000050UL;   // ldr x16, [pc, #8]
    pCode[1] = 0xD61F0200UL;   // br  x16

    // Ensure that the updated instructions get updated in the I-Cache
    ClrFlushInstructionCache(pCode, 8);

    *((LPVOID *)(pCode + 2)) = target;   // 64-bit target address

}

//------------------------------------------------------------------------
//  Given the same pBuffer that was used by emitJump this method
//  decodes the instructions and returns the jump target
inline PCODE decodeJump(PCODE pCode)
{
    LIMITED_METHOD_CONTRACT;

    TADDR pInstr = PCODEToPINSTR(pCode);

    return *dac_cast<PTR_PCODE>(pInstr + 2*sizeof(DWORD));
}

//------------------------------------------------------------------------
inline BOOL isJump(PCODE pCode)
{
    LIMITED_METHOD_DAC_CONTRACT;

    TADDR pInstr = PCODEToPINSTR(pCode);

    return *dac_cast<PTR_DWORD>(pInstr) == 0x58000050;
}

//------------------------------------------------------------------------
inline BOOL isBackToBackJump(PCODE pBuffer)
{
    WRAPPER_NO_CONTRACT;
    SUPPORTS_DAC;
    return isJump(pBuffer);
}

//------------------------------------------------------------------------
inline void emitBackToBackJump(LPBYTE pBuffer, LPVOID target)
{
    WRAPPER_NO_CONTRACT;
    emitJump((UINT32*)pBuffer, target);
}

//------------------------------------------------------------------------
inline PCODE decodeBackToBackJump(PCODE pBuffer)
{
    WRAPPER_NO_CONTRACT;
    return decodeJump(pBuffer);
}

// SEH info forward declarations

inline BOOL IsUnmanagedValueTypeReturnedByRef(UINT sizeofvaluetype) 
{
//   ARM64TODO : Check if we need to consider HFA
    return (sizeofvaluetype > 8);
}


//----------------------------------------------------------------------

struct IntReg
{
    int reg;
    IntReg(int reg):reg(reg)
    {
        _ASSERTE(0 <= reg && reg < 32);
    }

    operator int () { return reg; }
    operator int () const { return reg; }
    int operator == (IntReg other) { return reg == other.reg; }
    int operator != (IntReg other) { return reg != other.reg; }
    WORD Mask() const { return 1 << reg; }
};

struct VecReg
{
    int reg;
    VecReg(int reg):reg(reg)
    {
        _ASSERTE(0 <= reg && reg < 32);
    }

    operator int() { return reg; }
    int operator == (VecReg other) { return reg == other.reg; }
    int operator != (VecReg other) { return reg != other.reg; }
    WORD Mask() const { return 1 << reg; }
};

struct CondCode
{
    int cond;
    CondCode(int cond):cond(cond)
    {
        _ASSERTE(0 <= cond && cond < 16);
    }
};

const IntReg RegTeb = IntReg(18);
const IntReg RegFp  = IntReg(29);
const IntReg RegLr  = IntReg(30);
// Note that stack pointer and zero register share the same encoding, 31 
const IntReg RegSp  = IntReg(31);

const CondCode CondEq = CondCode(0);
const CondCode CondNe = CondCode(1);
const CondCode CondCs = CondCode(2);
const CondCode CondCc = CondCode(3);
const CondCode CondMi = CondCode(4);
const CondCode CondPl = CondCode(5);
const CondCode CondVs = CondCode(6);
const CondCode CondVc = CondCode(7);
const CondCode CondHi = CondCode(8);
const CondCode CondLs = CondCode(9);
const CondCode CondGe = CondCode(10);
const CondCode CondLt = CondCode(11);
const CondCode CondGt = CondCode(12);
const CondCode CondLe = CondCode(13);
const CondCode CondAl = CondCode(14);
const CondCode CondNv = CondCode(15);


#define PRECODE_ALIGNMENT           CODE_SIZE_ALIGN
#define SIZEOF_PRECODE_BASE         CODE_SIZE_ALIGN
#define OFFSETOF_PRECODE_TYPE       0

#ifdef CROSSGEN_COMPILE
#define GetEEFuncEntryPoint(pfn) 0x1001
#else
#define GetEEFuncEntryPoint(pfn) GFN_TADDR(pfn)
#endif

class StubLinkerCPU : public StubLinker
{

private:
    void EmitLoadStoreRegPairImm(DWORD flags, int regNum1, int regNum2, IntReg Xn, int offset, BOOL isVec);
    void EmitLoadStoreRegImm(DWORD flags, int regNum, IntReg Xn, int offset, BOOL isVec);
public:

    // BitFlags for EmitLoadStoreReg(Pair)Imm methods
    enum {
        eSTORE      =   0x0,
        eLOAD       =   0x1,
        eWRITEBACK  =   0x2,
        ePOSTINDEX  =   0x4,
        eFLAGMASK   =   0x7
    };

    // BitFlags for Register offsetted loads/stores
    // Bits(1-3) indicate the <extend> encoding, while the bits(0) indicate the shift
    enum {
        eSHIFT      =   0x1, // 0y0001
        eUXTW       =   0x4, // 0y0100
        eSXTW       =   0xC, // 0y1100
        eLSL        =   0x7, // 0y0111
        eSXTX       =   0xD, // 0y1110
    };


    static void Init(); 
    
    void EmitUnboxMethodStub(MethodDesc* pRealMD);
    void EmitCallManagedMethod(MethodDesc *pMD, BOOL fTailCall);
    void EmitCallLabel(CodeLabel *target, BOOL fTailCall, BOOL fIndirect);
    void EmitSecureDelegateInvoke(UINT_PTR hash);
    static UINT_PTR HashMulticastInvoke(MetaSig* pSig);
    void EmitShuffleThunk(struct ShuffleEntry *pShuffleEntryArray);
    void EmitGetThreadInlined(IntReg Xt);

#ifdef _DEBUG
    void EmitNop() { Emit32(0xD503201F); }
#endif
    void EmitBreakPoint() { Emit32(0xD43E0000); }
    void EmitMovConstant(IntReg target, UINT64 constant);
    void EmitCmpImm(IntReg reg, int imm);
    void EmitCmpReg(IntReg Xn, IntReg Xm);
    void EmitCondFlagJump(CodeLabel * target, UINT cond);
    void EmitJumpRegister(IntReg regTarget);
    void EmitMovReg(IntReg dest, IntReg source);

    void EmitSubImm(IntReg Xd, IntReg Xn, unsigned int value);
    void EmitAddImm(IntReg Xd, IntReg Xn, unsigned int value);

    void EmitLoadStoreRegPairImm(DWORD flags, IntReg Xt1, IntReg Xt2, IntReg Xn, int offset=0);
    void EmitLoadStoreRegPairImm(DWORD flags, VecReg Vt1, VecReg Vt2, IntReg Xn, int offset=0);

    void EmitLoadStoreRegImm(DWORD flags, IntReg Xt, IntReg Xn, int offset=0);
    void EmitLoadStoreRegImm(DWORD flags, VecReg Vt, IntReg Xn, int offset=0);

    void EmitLoadRegReg(IntReg Xt, IntReg Xn, IntReg Xm, DWORD option);

    void EmitCallRegister(IntReg reg);
    void EmitProlog(unsigned short cIntRegArgs, 
                    unsigned short cVecRegArgs, 
                    unsigned short cCalleeSavedRegs,
                    unsigned short cbStackSpace = 0); 

    void EmitEpilog();

    void EmitRet(IntReg reg);


};

extern "C" void SinglecastDelegateInvokeStub();


// preferred alignment for data
//ARM64TODO: double check
#define DATA_ALIGNMENT 8


struct DECLSPEC_ALIGN(16) UMEntryThunkCode
{
    DWORD        m_code[4];

    TADDR       m_pTargetCode;
    TADDR       m_pvSecretParam;

    void Encode(BYTE* pTargetCode, void* pvSecretParam);

    LPCBYTE GetEntryPoint() const
    {
        LIMITED_METHOD_CONTRACT;

        return (LPCBYTE)this;
    }

    static int GetEntryPointOffset()
    {
        LIMITED_METHOD_CONTRACT;

        return 0;
    }
};

struct HijackArgs
{
    // ARM64:NYI
};

EXTERN_C VOID STDCALL PrecodeFixupThunk();

// Invalid precode type
struct InvalidPrecode {
    static const int Type = 0;
};

struct StubPrecode {

    static const int Type = 0x89;

    // adr x9, #16   
    // ldp x10,x12,[x9]      ; =m_pTarget,m_pMethodDesc
    // br x10
    // 4 byte padding for 8 byte allignement
    // dcd pTarget
    // dcd pMethodDesc
    DWORD   m_rgCode[4];
    TADDR   m_pTarget;
    TADDR   m_pMethodDesc;

    void Init(MethodDesc* pMD, LoaderAllocator *pLoaderAllocator);

    TADDR GetMethodDesc()
    {
        LIMITED_METHOD_DAC_CONTRACT; 
        return m_pMethodDesc;
    }

    PCODE GetTarget()
    {
        LIMITED_METHOD_DAC_CONTRACT; 
        return m_pTarget;
    }

    BOOL SetTargetInterlocked(TADDR target, TADDR expected)
    {
        CONTRACTL
        {
            THROWS;
            GC_TRIGGERS;
        }
        CONTRACTL_END;

        EnsureWritableExecutablePages(&m_pTarget);
        return (TADDR)InterlockedCompareExchange64(
            (LONGLONG*)&m_pTarget, (TADDR)target, (TADDR)expected) == expected;
    }

#ifdef FEATURE_PREJIT
    void Fixup(DataImage *image);
#endif
};
typedef DPTR(StubPrecode) PTR_StubPrecode;


struct NDirectImportPrecode {

    static const int Type = 0x88;

    // adr x8, #16             ; Notice that x8 register is used to differentiate the stub from StubPrecode which uses x9
    // ldp x10,x12,[x8]      ; =m_pTarget,m_pMethodDesc
    // br x10
    // 4 byte padding for 8 byte allignement
    // dcd pTarget
    // dcd pMethodDesc
    DWORD    m_rgCode[4];
    TADDR   m_pTarget;
    TADDR   m_pMethodDesc;

    void Init(MethodDesc* pMD, LoaderAllocator *pLoaderAllocator);

    TADDR GetMethodDesc()
    {
        LIMITED_METHOD_DAC_CONTRACT; 
        return m_pMethodDesc;
    }

    PCODE GetTarget()
    {
        LIMITED_METHOD_DAC_CONTRACT; 
        return m_pTarget;
    }

    LPVOID GetEntrypoint()
    {
        LIMITED_METHOD_CONTRACT;
        return this;
    }

#ifdef FEATURE_PREJIT
    void Fixup(DataImage *image);
#endif
};
typedef DPTR(NDirectImportPrecode) PTR_NDirectImportPrecode;


struct FixupPrecode {

    static const int Type = 0xfc;

    // mov r12, pc
    // ldr pc, [pc, #4]     ; =m_pTarget
    // dcb m_MethodDescChunkIndex
    // dcb m_PrecodeChunkIndex
    // dcd m_pTarget
    WORD    m_rgCode[3];
    BYTE    m_MethodDescChunkIndex;
    BYTE    m_PrecodeChunkIndex;
    TADDR   m_pTarget;

    void Init(MethodDesc* pMD, LoaderAllocator *pLoaderAllocator, int iMethodDescChunkIndex = 0, int iPrecodeChunkIndex = 0);

    TADDR GetBase()
    {
        _ASSERTE(!"ARM64:NYI");
        return NULL;
    }

    TADDR GetMethodDesc();

    PCODE GetTarget()
    {
        _ASSERTE(!"ARM64:NYI");
        return NULL;
    }

    BOOL SetTargetInterlocked(TADDR target, TADDR expected)
    {
        _ASSERTE(!"ARM64:NYI");
        return NULL;
    }

    static BOOL IsFixupPrecodeByASM(PCODE addr)
    {
        _ASSERTE(!"ARM64:NYI");
        return NULL;
    }

#ifdef FEATURE_PREJIT
    // Partial initialization. Used to save regrouped chunks.
    void InitForSave(int iPrecodeChunkIndex);

    void Fixup(DataImage *image, MethodDesc * pMD);
#endif

#ifdef DACCESS_COMPILE
    void EnumMemoryRegions(CLRDataEnumMemoryFlags flags);
#endif
};
typedef DPTR(FixupPrecode) PTR_FixupPrecode;


// Precode to stuffle this and retbuf for closed delegates over static methods with return buffer
struct ThisPtrRetBufPrecode {

    static const int Type = 0x84;

    // mov r12, r0
    // mov r0, r1
    // mov r1, r12
    // ldr pc, [pc, #0]     ; =m_pTarget
    // dcd pTarget
    // dcd pMethodDesc
    WORD    m_rgCode[6];
    TADDR   m_pTarget;
    TADDR   m_pMethodDesc;

    void Init(MethodDesc* pMD, LoaderAllocator *pLoaderAllocator);

    TADDR GetMethodDesc()
    {
        _ASSERTE(!"ARM64:NYI");
        return NULL;
    }

    PCODE GetTarget()
    { 
        _ASSERTE(!"ARM64:NYI");
        return NULL;
    }

    BOOL SetTargetInterlocked(TADDR target, TADDR expected)
    {
        _ASSERTE(!"ARM64:NYI");
        return NULL;
    }
};
typedef DPTR(ThisPtrRetBufPrecode) PTR_ThisPtrRetBufPrecode;


#ifdef HAS_REMOTING_PRECODE

// Precode with embedded remoting interceptor
struct RemotingPrecode {

    static const int Type = 0x02;

    // push {r1,lr}
    // ldr r1, [pc, #16]    ; =m_pPrecodeRemotingThunk
    // blx r1
    // pop {r1,lr}
    // ldr pc, [pc, #12]    ; =m_pLocalTarget
    // nop                  ; padding for alignment
    // dcd m_pMethodDesc
    // dcd m_pPrecodeRemotingThunk
    // dcd m_pLocalTarget
    WORD    m_rgCode[8];
    TADDR   m_pMethodDesc;
    TADDR   m_pPrecodeRemotingThunk;
    TADDR   m_pLocalTarget;

    void Init(MethodDesc* pMD, LoaderAllocator *pLoaderAllocator = NULL);

    TADDR GetMethodDesc()
    {
        _ASSERTE(!"ARM64:NYI");
        return NULL;
    }

    PCODE GetTarget()
    { 
        _ASSERTE(!"ARM64:NYI");
        return NULL;
    }

    BOOL SetTargetInterlocked(TADDR target, TADDR expected)
    {
        _ASSERTE(!"ARM64:NYI");
        return NULL;
    }

#ifdef FEATURE_PREJIT
    void Fixup(DataImage *image, ZapNode *pCodeNode);
#endif
};
typedef DPTR(RemotingPrecode) PTR_RemotingPrecode;

EXTERN_C void PrecodeRemotingThunk();

#endif // HAS_REMOTING_PRECODE


#endif // __cgencpu_h__
