//===-- PPCFrameInfo.h - Define TargetFrameInfo for PowerPC -----*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
//
//===----------------------------------------------------------------------===//

#ifndef POWERPC_FRAMEINFO_H
#define POWERPC_FRAMEINFO_H

#include "PPC.h"
#include "PPCSubtarget.h"
#include "llvm/Target/TargetFrameInfo.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/ADT/STLExtras.h"

namespace llvm {

class PPCFrameInfo: public TargetFrameInfo {
  const TargetMachine &TM;

public:
  PPCFrameInfo(const TargetMachine &tm, bool LP64)
    : TargetFrameInfo(TargetFrameInfo::StackGrowsDown, 16, 0), TM(tm) {
  }

  /// getReturnSaveOffset - Return the previous frame offset to save the
  /// return address.
  static unsigned getReturnSaveOffset(bool isPPC64, bool isDarwinABI) {
    if (isDarwinABI)
      return isPPC64 ? 16 : 8;
    // SVR4 ABI:
    return isPPC64 ? 16 : 4;
  }

  /// getFramePointerSaveOffset - Return the previous frame offset to save the
  /// frame pointer.
  static unsigned getFramePointerSaveOffset(bool isPPC64, bool isDarwinABI) {
    // For the Darwin ABI:
    // We cannot use the TOC save slot (offset +20) in the PowerPC linkage area
    // for saving the frame pointer (if needed.)  While the published ABI has
    // not used this slot since at least MacOSX 10.2, there is older code
    // around that does use it, and that needs to continue to work.
    if (isDarwinABI)
      return isPPC64 ? -8U : -4U;
    
    // SVR4 ABI: First slot in the general register save area.
    return isPPC64 ? -8U : -4U;
  }
  
  /// getLinkageSize - Return the size of the PowerPC ABI linkage area.
  ///
  static unsigned getLinkageSize(bool isPPC64, bool isDarwinABI) {
    if (isDarwinABI || isPPC64)
      return 6 * (isPPC64 ? 8 : 4);
    
    // SVR4 ABI:
    return 8;
  }

  /// getMinCallArgumentsSize - Return the size of the minium PowerPC ABI
  /// argument area.
  static unsigned getMinCallArgumentsSize(bool isPPC64, bool isDarwinABI) {
    // For the Darwin ABI / 64-bit SVR4 ABI:
    // The prolog code of the callee may store up to 8 GPR argument registers to
    // the stack, allowing va_start to index over them in memory if its varargs.
    // Because we cannot tell if this is needed on the caller side, we have to
    // conservatively assume that it is needed.  As such, make sure we have at
    // least enough stack space for the caller to store the 8 GPRs.
    if (isDarwinABI || isPPC64)
      return 8 * (isPPC64 ? 8 : 4);
    
    // 32-bit SVR4 ABI:
    // There is no default stack allocated for the 8 first GPR arguments.
    return 0;
  }

  /// getMinCallFrameSize - Return the minimum size a call frame can be using
  /// the PowerPC ABI.
  static unsigned getMinCallFrameSize(bool isPPC64, bool isDarwinABI) {
    // The call frame needs to be at least big enough for linkage and 8 args.
    return getLinkageSize(isPPC64, isDarwinABI) +
           getMinCallArgumentsSize(isPPC64, isDarwinABI);
  }

  // With the SVR4 ABI, callee-saved registers have fixed offsets on the stack.
  const SpillSlot *
  getCalleeSavedSpillSlots(unsigned &NumEntries) const {
    if (TM.getSubtarget<PPCSubtarget>().isDarwinABI()) {
      NumEntries = 1;
      if (TM.getSubtarget<PPCSubtarget>().isPPC64()) {
        static const SpillSlot darwin64Offsets = {PPC::X31, -8};
        return &darwin64Offsets;
      } else {
        static const SpillSlot darwinOffsets = {PPC::R31, -4};
        return &darwinOffsets;
      }
    }

    // Early exit if not using the SVR4 ABI.
    if (!TM.getSubtarget<PPCSubtarget>().isSVR4ABI()) {
      NumEntries = 0;
      return 0;
    }

    static const SpillSlot Offsets[] = {
      // Floating-point register save area offsets.
      {PPC::F31, -8},
      {PPC::F30, -16},
      {PPC::F29, -24},
      {PPC::F28, -32},
      {PPC::F27, -40},
      {PPC::F26, -48},
      {PPC::F25, -56},
      {PPC::F24, -64},
      {PPC::F23, -72},
      {PPC::F22, -80},
      {PPC::F21, -88},
      {PPC::F20, -96},
      {PPC::F19, -104},
      {PPC::F18, -112},
      {PPC::F17, -120},
      {PPC::F16, -128},
      {PPC::F15, -136},
      {PPC::F14, -144},

      // General register save area offsets.
      {PPC::R31, -4},
      {PPC::R30, -8},
      {PPC::R29, -12},
      {PPC::R28, -16},
      {PPC::R27, -20},
      {PPC::R26, -24},
      {PPC::R25, -28},
      {PPC::R24, -32},
      {PPC::R23, -36},
      {PPC::R22, -40},
      {PPC::R21, -44},
      {PPC::R20, -48},
      {PPC::R19, -52},
      {PPC::R18, -56},
      {PPC::R17, -60},
      {PPC::R16, -64},
      {PPC::R15, -68},
      {PPC::R14, -72},

      // CR save area offset.
      // FIXME SVR4: Disable CR save area for now.
//      {PPC::CR2, -4},
//      {PPC::CR3, -4},
//      {PPC::CR4, -4},
//      {PPC::CR2LT, -4},
//      {PPC::CR2GT, -4},
//      {PPC::CR2EQ, -4},
//      {PPC::CR2UN, -4},
//      {PPC::CR3LT, -4},
//      {PPC::CR3GT, -4},
//      {PPC::CR3EQ, -4},
//      {PPC::CR3UN, -4},
//      {PPC::CR4LT, -4},
//      {PPC::CR4GT, -4},
//      {PPC::CR4EQ, -4},
//      {PPC::CR4UN, -4},

      // VRSAVE save area offset.
      {PPC::VRSAVE, -4},

      // Vector register save area
      {PPC::V31, -16},
      {PPC::V30, -32},
      {PPC::V29, -48},
      {PPC::V28, -64},
      {PPC::V27, -80},
      {PPC::V26, -96},
      {PPC::V25, -112},
      {PPC::V24, -128},
      {PPC::V23, -144},
      {PPC::V22, -160},
      {PPC::V21, -176},
      {PPC::V20, -192}
    };

    static const SpillSlot Offsets64[] = {
      // Floating-point register save area offsets.
      {PPC::F31, -8},
      {PPC::F30, -16},
      {PPC::F29, -24},
      {PPC::F28, -32},
      {PPC::F27, -40},
      {PPC::F26, -48},
      {PPC::F25, -56},
      {PPC::F24, -64},
      {PPC::F23, -72},
      {PPC::F22, -80},
      {PPC::F21, -88},
      {PPC::F20, -96},
      {PPC::F19, -104},
      {PPC::F18, -112},
      {PPC::F17, -120},
      {PPC::F16, -128},
      {PPC::F15, -136},
      {PPC::F14, -144},

      // General register save area offsets.
      // FIXME 64-bit SVR4: Are 32-bit registers actually allocated in 64-bit
      //                    mode?
      {PPC::R31, -4},
      {PPC::R30, -12},
      {PPC::R29, -20},
      {PPC::R28, -28},
      {PPC::R27, -36},
      {PPC::R26, -44},
      {PPC::R25, -52},
      {PPC::R24, -60},
      {PPC::R23, -68},
      {PPC::R22, -76},
      {PPC::R21, -84},
      {PPC::R20, -92},
      {PPC::R19, -100},
      {PPC::R18, -108},
      {PPC::R17, -116},
      {PPC::R16, -124},
      {PPC::R15, -132},
      {PPC::R14, -140},

      {PPC::X31, -8},
      {PPC::X30, -16},
      {PPC::X29, -24},
      {PPC::X28, -32},
      {PPC::X27, -40},
      {PPC::X26, -48},
      {PPC::X25, -56},
      {PPC::X24, -64},
      {PPC::X23, -72},
      {PPC::X22, -80},
      {PPC::X21, -88},
      {PPC::X20, -96},
      {PPC::X19, -104},
      {PPC::X18, -112},
      {PPC::X17, -120},
      {PPC::X16, -128},
      {PPC::X15, -136},
      {PPC::X14, -144},

      // CR save area offset.
      // FIXME SVR4: Disable CR save area for now.
//      {PPC::CR2, -4},
//      {PPC::CR3, -4},
//      {PPC::CR4, -4},
//      {PPC::CR2LT, -4},
//      {PPC::CR2GT, -4},
//      {PPC::CR2EQ, -4},
//      {PPC::CR2UN, -4},
//      {PPC::CR3LT, -4},
//      {PPC::CR3GT, -4},
//      {PPC::CR3EQ, -4},
//      {PPC::CR3UN, -4},
//      {PPC::CR4LT, -4},
//      {PPC::CR4GT, -4},
//      {PPC::CR4EQ, -4},
//      {PPC::CR4UN, -4},

      // VRSAVE save area offset.
      {PPC::VRSAVE, -4},

      // Vector register save area
      {PPC::V31, -16},
      {PPC::V30, -32},
      {PPC::V29, -48},
      {PPC::V28, -64},
      {PPC::V27, -80},
      {PPC::V26, -96},
      {PPC::V25, -112},
      {PPC::V24, -128},
      {PPC::V23, -144},
      {PPC::V22, -160},
      {PPC::V21, -176},
      {PPC::V20, -192}
    };

    if (TM.getSubtarget<PPCSubtarget>().isPPC64()) {
      NumEntries = array_lengthof(Offsets64);

      return Offsets64;
    } else {
      NumEntries = array_lengthof(Offsets);

      return Offsets;
    }
  }
};

} // End llvm namespace

#endif
