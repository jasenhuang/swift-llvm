//===- DWARFFormValue.cpp -------------------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "llvm/DebugInfo/DWARF/DWARFFormValue.h"
#include "SyntaxHighlighting.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/None.h"
#include "llvm/ADT/Optional.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/BinaryFormat/Dwarf.h"
#include "llvm/DebugInfo/DWARF/DWARFContext.h"
#include "llvm/DebugInfo/DWARF/DWARFRelocMap.h"
#include "llvm/DebugInfo/DWARF/DWARFUnit.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/Format.h"
#include "llvm/Support/raw_ostream.h"
#include <cinttypes>
#include <cstdint>
#include <limits>

using namespace llvm;
using namespace dwarf;
using namespace syntax;

static const DWARFFormValue::FormClass DWARF4FormClasses[] = {
    DWARFFormValue::FC_Unknown,  // 0x0
    DWARFFormValue::FC_Address,  // 0x01 DW_FORM_addr
    DWARFFormValue::FC_Unknown,  // 0x02 unused
    DWARFFormValue::FC_Block,    // 0x03 DW_FORM_block2
    DWARFFormValue::FC_Block,    // 0x04 DW_FORM_block4
    DWARFFormValue::FC_Constant, // 0x05 DW_FORM_data2
    // --- These can be FC_SectionOffset in DWARF3 and below:
    DWARFFormValue::FC_Constant, // 0x06 DW_FORM_data4
    DWARFFormValue::FC_Constant, // 0x07 DW_FORM_data8
    // ---
    DWARFFormValue::FC_String,        // 0x08 DW_FORM_string
    DWARFFormValue::FC_Block,         // 0x09 DW_FORM_block
    DWARFFormValue::FC_Block,         // 0x0a DW_FORM_block1
    DWARFFormValue::FC_Constant,      // 0x0b DW_FORM_data1
    DWARFFormValue::FC_Flag,          // 0x0c DW_FORM_flag
    DWARFFormValue::FC_Constant,      // 0x0d DW_FORM_sdata
    DWARFFormValue::FC_String,        // 0x0e DW_FORM_strp
    DWARFFormValue::FC_Constant,      // 0x0f DW_FORM_udata
    DWARFFormValue::FC_Reference,     // 0x10 DW_FORM_ref_addr
    DWARFFormValue::FC_Reference,     // 0x11 DW_FORM_ref1
    DWARFFormValue::FC_Reference,     // 0x12 DW_FORM_ref2
    DWARFFormValue::FC_Reference,     // 0x13 DW_FORM_ref4
    DWARFFormValue::FC_Reference,     // 0x14 DW_FORM_ref8
    DWARFFormValue::FC_Reference,     // 0x15 DW_FORM_ref_udata
    DWARFFormValue::FC_Indirect,      // 0x16 DW_FORM_indirect
    DWARFFormValue::FC_SectionOffset, // 0x17 DW_FORM_sec_offset
    DWARFFormValue::FC_Exprloc,       // 0x18 DW_FORM_exprloc
    DWARFFormValue::FC_Flag,          // 0x19 DW_FORM_flag_present
};

Optional<uint8_t>
DWARFFormValue::getFixedByteSize(dwarf::Form Form,
                                 const DWARFFormParams Params) {
  switch (Form) {
  case DW_FORM_addr:
    assert(Params.Version && Params.AddrSize && "Invalid Params for form");
    return Params.AddrSize;

  case DW_FORM_block:          // ULEB128 length L followed by L bytes.
  case DW_FORM_block1:         // 1 byte length L followed by L bytes.
  case DW_FORM_block2:         // 2 byte length L followed by L bytes.
  case DW_FORM_block4:         // 4 byte length L followed by L bytes.
  case DW_FORM_string:         // C-string with null terminator.
  case DW_FORM_sdata:          // SLEB128.
  case DW_FORM_udata:          // ULEB128.
  case DW_FORM_ref_udata:      // ULEB128.
  case DW_FORM_indirect:       // ULEB128.
  case DW_FORM_exprloc:        // ULEB128 length L followed by L bytes.
  case DW_FORM_strx:           // ULEB128.
  case DW_FORM_addrx:          // ULEB128.
  case DW_FORM_loclistx:       // ULEB128.
  case DW_FORM_rnglistx:       // ULEB128.
  case DW_FORM_GNU_addr_index: // ULEB128.
  case DW_FORM_GNU_str_index:  // ULEB128.
    return None;

  case DW_FORM_ref_addr:
    assert(Params.Version && Params.AddrSize && "Invalid Params for form");
    return Params.getRefAddrByteSize();

  case DW_FORM_flag:
  case DW_FORM_data1:
  case DW_FORM_ref1:
  case DW_FORM_strx1:
  case DW_FORM_addrx1:
    return 1;

  case DW_FORM_data2:
  case DW_FORM_ref2:
  case DW_FORM_strx2:
  case DW_FORM_addrx2:
    return 2;

  case DW_FORM_strx3:
    return 3;

  case DW_FORM_data4:
  case DW_FORM_ref4:
  case DW_FORM_ref_sup4:
  case DW_FORM_strx4:
  case DW_FORM_addrx4:
    return 4;

  case DW_FORM_strp:
  case DW_FORM_GNU_ref_alt:
  case DW_FORM_GNU_strp_alt:
  case DW_FORM_line_strp:
  case DW_FORM_sec_offset:
  case DW_FORM_strp_sup:
    assert(Params.Version && Params.AddrSize && "Invalid Params for form");
    return Params.getDwarfOffsetByteSize();

  case DW_FORM_data8:
  case DW_FORM_ref8:
  case DW_FORM_ref_sig8:
  case DW_FORM_ref_sup8:
    return 8;

  case DW_FORM_flag_present:
    return 0;

  case DW_FORM_data16:
    return 16;

  case DW_FORM_implicit_const:
    // The implicit value is stored in the abbreviation as a SLEB128, and
    // there no data in debug info.
    return 0;

  default:
    llvm_unreachable("Handle this form in this switch statement");
  }
  return None;
}

bool DWARFFormValue::skipValue(dwarf::Form Form, DataExtractor DebugInfoData,
                               uint32_t *OffsetPtr,
                               const DWARFFormParams Params) {
  bool Indirect = false;
  do {
    switch (Form) {
    // Blocks of inlined data that have a length field and the data bytes
    // inlined in the .debug_info.
    case DW_FORM_exprloc:
    case DW_FORM_block: {
      uint64_t size = DebugInfoData.getULEB128(OffsetPtr);
      *OffsetPtr += size;
      return true;
    }
    case DW_FORM_block1: {
      uint8_t size = DebugInfoData.getU8(OffsetPtr);
      *OffsetPtr += size;
      return true;
    }
    case DW_FORM_block2: {
      uint16_t size = DebugInfoData.getU16(OffsetPtr);
      *OffsetPtr += size;
      return true;
    }
    case DW_FORM_block4: {
      uint32_t size = DebugInfoData.getU32(OffsetPtr);
      *OffsetPtr += size;
      return true;
    }

    // Inlined NULL terminated C-strings.
    case DW_FORM_string:
      DebugInfoData.getCStr(OffsetPtr);
      return true;

    case DW_FORM_addr:
    case DW_FORM_ref_addr:
    case DW_FORM_flag_present:
    case DW_FORM_data1:
    case DW_FORM_data2:
    case DW_FORM_data4:
    case DW_FORM_data8:
    case DW_FORM_data16:
    case DW_FORM_flag:
    case DW_FORM_ref1:
    case DW_FORM_ref2:
    case DW_FORM_ref4:
    case DW_FORM_ref8:
    case DW_FORM_ref_sig8:
    case DW_FORM_ref_sup4:
    case DW_FORM_ref_sup8:
    case DW_FORM_strx1:
    case DW_FORM_strx2:
    case DW_FORM_strx4:
    case DW_FORM_addrx1:
    case DW_FORM_addrx2:
    case DW_FORM_addrx4:
    case DW_FORM_sec_offset:
    case DW_FORM_strp:
    case DW_FORM_strp_sup:
    case DW_FORM_line_strp:
    case DW_FORM_GNU_ref_alt:
    case DW_FORM_GNU_strp_alt:
      if (Optional<uint8_t> FixedSize =
              DWARFFormValue::getFixedByteSize(Form, Params)) {
        *OffsetPtr += *FixedSize;
        return true;
      }
      return false;

    // signed or unsigned LEB 128 values.
    case DW_FORM_sdata:
      DebugInfoData.getSLEB128(OffsetPtr);
      return true;

    case DW_FORM_udata:
    case DW_FORM_ref_udata:
    case DW_FORM_strx:
    case DW_FORM_addrx:
    case DW_FORM_loclistx:
    case DW_FORM_rnglistx:
    case DW_FORM_GNU_addr_index:
    case DW_FORM_GNU_str_index:
      DebugInfoData.getULEB128(OffsetPtr);
      return true;

    case DW_FORM_indirect:
      Indirect = true;
      Form = static_cast<dwarf::Form>(DebugInfoData.getULEB128(OffsetPtr));
      break;

    default:
      return false;
    }
  } while (Indirect);
  return true;
}

bool DWARFFormValue::isFormClass(DWARFFormValue::FormClass FC) const {
  // First, check DWARF4 form classes.
  if (Form < makeArrayRef(DWARF4FormClasses).size() &&
      DWARF4FormClasses[Form] == FC)
    return true;
  // Check more forms from DWARF4 and DWARF5 proposals.
  switch (Form) {
  case DW_FORM_ref_sig8:
  case DW_FORM_GNU_ref_alt:
    return (FC == FC_Reference);
  case DW_FORM_GNU_addr_index:
    return (FC == FC_Address);
  case DW_FORM_GNU_str_index:
  case DW_FORM_GNU_strp_alt:
  case DW_FORM_strx:
  case DW_FORM_strx1:
  case DW_FORM_strx2:
  case DW_FORM_strx3:
  case DW_FORM_strx4:
    return (FC == FC_String);
  case DW_FORM_implicit_const:
    return (FC == FC_Constant);
  default:
    break;
  }
  // In DWARF3 DW_FORM_data4 and DW_FORM_data8 served also as a section offset.
  // Don't check for DWARF version here, as some producers may still do this
  // by mistake. Also accept DW_FORM_strp since this is .debug_str section
  // offset.
  return (Form == DW_FORM_data4 || Form == DW_FORM_data8 ||
          Form == DW_FORM_strp) &&
         FC == FC_SectionOffset;
}

bool DWARFFormValue::extractValue(const DWARFDataExtractor &Data,
                                  uint32_t *OffsetPtr, DWARFFormParams FP,
                                  const DWARFUnit *CU) {
  U = CU;
  bool Indirect = false;
  bool IsBlock = false;
  Value.data = nullptr;
  // Read the value for the form into value and follow and DW_FORM_indirect
  // instances we run into
  do {
    Indirect = false;
    switch (Form) {
    case DW_FORM_addr:
    case DW_FORM_ref_addr: {
      uint16_t Size =
          (Form == DW_FORM_addr) ? FP.AddrSize : FP.getRefAddrByteSize();
      Value.uval = Data.getRelocatedValue(Size, OffsetPtr, &Value.SectionIndex);
      break;
    }
    case DW_FORM_exprloc:
    case DW_FORM_block:
      Value.uval = Data.getULEB128(OffsetPtr);
      IsBlock = true;
      break;
    case DW_FORM_block1:
      Value.uval = Data.getU8(OffsetPtr);
      IsBlock = true;
      break;
    case DW_FORM_block2:
      Value.uval = Data.getU16(OffsetPtr);
      IsBlock = true;
      break;
    case DW_FORM_block4:
      Value.uval = Data.getU32(OffsetPtr);
      IsBlock = true;
      break;
    case DW_FORM_data1:
    case DW_FORM_ref1:
    case DW_FORM_flag:
    case DW_FORM_strx1:
    case DW_FORM_addrx1:
      Value.uval = Data.getU8(OffsetPtr);
      break;
    case DW_FORM_data2:
    case DW_FORM_ref2:
    case DW_FORM_strx2:
    case DW_FORM_addrx2:
      Value.uval = Data.getU16(OffsetPtr);
      break;
    case DW_FORM_strx3:
      Value.uval = Data.getU24(OffsetPtr);
      break;
    case DW_FORM_data4:
    case DW_FORM_ref4:
    case DW_FORM_ref_sup4:
    case DW_FORM_strx4:
    case DW_FORM_addrx4:
      Value.uval = Data.getRelocatedValue(4, OffsetPtr);
      break;
    case DW_FORM_data8:
    case DW_FORM_ref8:
    case DW_FORM_ref_sup8:
      Value.uval = Data.getU64(OffsetPtr);
      break;
    case DW_FORM_data16:
      // Treat this like a 16-byte block.
      Value.uval = 16;
      IsBlock = true;
      break;
    case DW_FORM_sdata:
      Value.sval = Data.getSLEB128(OffsetPtr);
      break;
    case DW_FORM_udata:
    case DW_FORM_ref_udata:
      Value.uval = Data.getULEB128(OffsetPtr);
      break;
    case DW_FORM_string:
      Value.cstr = Data.getCStr(OffsetPtr);
      break;
    case DW_FORM_indirect:
      Form = static_cast<dwarf::Form>(Data.getULEB128(OffsetPtr));
      Indirect = true;
      break;
    case DW_FORM_strp:
    case DW_FORM_sec_offset:
    case DW_FORM_GNU_ref_alt:
    case DW_FORM_GNU_strp_alt:
    case DW_FORM_line_strp:
    case DW_FORM_strp_sup: {
      Value.uval =
          Data.getRelocatedValue(FP.getDwarfOffsetByteSize(), OffsetPtr);
      break;
    }
    case DW_FORM_flag_present:
      Value.uval = 1;
      break;
    case DW_FORM_ref_sig8:
      Value.uval = Data.getU64(OffsetPtr);
      break;
    case DW_FORM_GNU_addr_index:
    case DW_FORM_GNU_str_index:
    case DW_FORM_strx:
      Value.uval = Data.getULEB128(OffsetPtr);
      break;
    default:
      // DWARFFormValue::skipValue() will have caught this and caused all
      // DWARF DIEs to fail to be parsed, so this code is not be reachable.
      llvm_unreachable("unsupported form");
    }
  } while (Indirect);

  if (IsBlock) {
    StringRef Str = Data.getData().substr(*OffsetPtr, Value.uval);
    Value.data = nullptr;
    if (!Str.empty()) {
      Value.data = reinterpret_cast<const uint8_t *>(Str.data());
      *OffsetPtr += Value.uval;
    }
  }

  return true;
}

void DWARFFormValue::dump(raw_ostream &OS, DIDumpOptions DumpOpts) const {
  uint64_t UValue = Value.uval;
  bool CURelativeOffset = false;
  raw_ostream &AddrOS =
      DumpOpts.ShowAddresses ? WithColor(OS, syntax::Address).get() : nulls();
  switch (Form) {
  case DW_FORM_addr:
    AddrOS << format("0x%016" PRIx64, UValue);
    break;
  case DW_FORM_GNU_addr_index: {
    AddrOS << format(" indexed (%8.8x) address = ", (uint32_t)UValue);
    uint64_t Address;
    if (U == nullptr)
      OS << "<invalid dwarf unit>";
    else if (U->getAddrOffsetSectionItem(UValue, Address))
      AddrOS << format("0x%016" PRIx64, Address);
    else
      OS << "<no .debug_addr section>";
    break;
  }
  case DW_FORM_flag_present:
    OS << "true";
    break;
  case DW_FORM_flag:
  case DW_FORM_data1:
    OS << format("0x%02x", (uint8_t)UValue);
    break;
  case DW_FORM_data2:
    OS << format("0x%04x", (uint16_t)UValue);
    break;
  case DW_FORM_data4:
    OS << format("0x%08x", (uint32_t)UValue);
    break;
  case DW_FORM_ref_sig8:
    AddrOS << format("0x%016" PRIx64, UValue);
    break;
  case DW_FORM_data8:
    OS << format("0x%016" PRIx64, UValue);
    break;
  case DW_FORM_data16:
    OS << format_bytes(ArrayRef<uint8_t>(Value.data, 16), None, 16, 16);
    break;
  case DW_FORM_string:
    OS << '"';
    OS.write_escaped(Value.cstr);
    OS << '"';
    break;
  case DW_FORM_exprloc:
  case DW_FORM_block:
  case DW_FORM_block1:
  case DW_FORM_block2:
  case DW_FORM_block4:
    if (UValue > 0) {
      switch (Form) {
      case DW_FORM_exprloc:
      case DW_FORM_block:
        OS << format("<0x%" PRIx64 "> ", UValue);
        break;
      case DW_FORM_block1:
        OS << format("<0x%2.2x> ", (uint8_t)UValue);
        break;
      case DW_FORM_block2:
        OS << format("<0x%4.4x> ", (uint16_t)UValue);
        break;
      case DW_FORM_block4:
        OS << format("<0x%8.8x> ", (uint32_t)UValue);
        break;
      default:
        break;
      }

      const uint8_t *DataPtr = Value.data;
      if (DataPtr) {
        // UValue contains size of block
        const uint8_t *EndDataPtr = DataPtr + UValue;
        while (DataPtr < EndDataPtr) {
          OS << format("%2.2x ", *DataPtr);
          ++DataPtr;
        }
      } else
        OS << "NULL";
    }
    break;

  case DW_FORM_sdata:
    OS << Value.sval;
    break;
  case DW_FORM_udata:
    OS << Value.uval;
    break;
  case DW_FORM_strp:
    if (DumpOpts.Verbose)
      OS << format(" .debug_str[0x%8.8x] = ", (uint32_t)UValue);
    dumpString(OS);
    break;
  case DW_FORM_strx:
  case DW_FORM_strx1:
  case DW_FORM_strx2:
  case DW_FORM_strx3:
  case DW_FORM_strx4:
  case DW_FORM_GNU_str_index:
    if (DumpOpts.Verbose)
      OS << format(" indexed (%8.8x) string = ", (uint32_t)UValue);
    dumpString(OS);
    break;
  case DW_FORM_GNU_strp_alt:
    if (DumpOpts.Verbose)
      OS << format("alt indirect string, offset: 0x%" PRIx64 "", UValue);
    dumpString(OS);
    break;
  case DW_FORM_ref_addr:
    AddrOS << format("0x%016" PRIx64, UValue);
    break;
  case DW_FORM_ref1:
    CURelativeOffset = true;
    if (DumpOpts.Verbose)
      AddrOS << format("cu + 0x%2.2x", (uint8_t)UValue);
    break;
  case DW_FORM_ref2:
    CURelativeOffset = true;
    if (DumpOpts.Verbose)
      AddrOS << format("cu + 0x%4.4x", (uint16_t)UValue);
    break;
  case DW_FORM_ref4:
    CURelativeOffset = true;
    if (DumpOpts.Verbose)
      AddrOS << format("cu + 0x%4.4x", (uint32_t)UValue);
    break;
  case DW_FORM_ref8:
    CURelativeOffset = true;
    if (DumpOpts.Verbose)
      AddrOS << format("cu + 0x%8.8" PRIx64, UValue);
    break;
  case DW_FORM_ref_udata:
    CURelativeOffset = true;
    if (DumpOpts.Verbose)
      AddrOS << format("cu + 0x%" PRIx64, UValue);
    break;
  case DW_FORM_GNU_ref_alt:
    AddrOS << format("<alt 0x%" PRIx64 ">", UValue);
    break;

  // All DW_FORM_indirect attributes should be resolved prior to calling
  // this function
  case DW_FORM_indirect:
    OS << "DW_FORM_indirect";
    break;

  // Should be formatted to 64-bit for DWARF64.
  case DW_FORM_sec_offset:
    AddrOS << format("0x%08x", (uint32_t)UValue);
    break;

  default:
    OS << format("DW_FORM(0x%4.4x)", Form);
    break;
  }

  if (CURelativeOffset) {
    if (DumpOpts.Verbose)
      OS << " => {";
    WithColor(OS, syntax::Address).get()
        << format("0x%8.8" PRIx64, UValue + (U ? U->getOffset() : 0));
    if (DumpOpts.Verbose)
      OS << "}";
  }
}

void DWARFFormValue::dumpString(raw_ostream &OS) const {
  Optional<const char *> DbgStr = getAsCString();
  if (DbgStr.hasValue()) {
    raw_ostream &COS = WithColor(OS, syntax::String);
    COS << '"';
    COS.write_escaped(DbgStr.getValue());
    COS << '"';
  }
}

Optional<const char *> DWARFFormValue::getAsCString() const {
  if (!isFormClass(FC_String))
    return None;
  if (Form == DW_FORM_string)
    return Value.cstr;
  // FIXME: Add support for DW_FORM_GNU_strp_alt
  if (Form == DW_FORM_GNU_strp_alt || U == nullptr)
    return None;
  uint32_t Offset = Value.uval;
  if (Form == DW_FORM_GNU_str_index || Form == DW_FORM_strx ||
      Form == DW_FORM_strx1 || Form == DW_FORM_strx2 || Form == DW_FORM_strx3 ||
      Form == DW_FORM_strx4) {
    uint64_t StrOffset;
    if (!U->getStringOffsetSectionItem(Offset, StrOffset))
      return None;
    Offset = StrOffset;
  }
  if (const char *Str = U->getStringExtractor().getCStr(&Offset)) {
    return Str;
  }
  return None;
}

Optional<uint64_t> DWARFFormValue::getAsAddress() const {
  if (!isFormClass(FC_Address))
    return None;
  if (Form == DW_FORM_GNU_addr_index) {
    uint32_t Index = Value.uval;
    uint64_t Result;
    if (!U || !U->getAddrOffsetSectionItem(Index, Result))
      return None;
    return Result;
  }
  return Value.uval;
}

Optional<uint64_t> DWARFFormValue::getAsReference() const {
  if (!isFormClass(FC_Reference))
    return None;
  switch (Form) {
  case DW_FORM_ref1:
  case DW_FORM_ref2:
  case DW_FORM_ref4:
  case DW_FORM_ref8:
  case DW_FORM_ref_udata:
    if (!U)
      return None;
    return Value.uval + U->getOffset();
  case DW_FORM_ref_addr:
  case DW_FORM_ref_sig8:
  case DW_FORM_GNU_ref_alt:
    return Value.uval;
  default:
    return None;
  }
}

Optional<uint64_t> DWARFFormValue::getAsSectionOffset() const {
  if (!isFormClass(FC_SectionOffset))
    return None;
  return Value.uval;
}

Optional<uint64_t> DWARFFormValue::getAsUnsignedConstant() const {
  if ((!isFormClass(FC_Constant) && !isFormClass(FC_Flag)) ||
      Form == DW_FORM_sdata)
    return None;
  return Value.uval;
}

Optional<int64_t> DWARFFormValue::getAsSignedConstant() const {
  if ((!isFormClass(FC_Constant) && !isFormClass(FC_Flag)) ||
      (Form == DW_FORM_udata &&
       uint64_t(std::numeric_limits<int64_t>::max()) < Value.uval))
    return None;
  switch (Form) {
  case DW_FORM_data4:
    return int32_t(Value.uval);
  case DW_FORM_data2:
    return int16_t(Value.uval);
  case DW_FORM_data1:
    return int8_t(Value.uval);
  case DW_FORM_sdata:
  case DW_FORM_data8:
  default:
    return Value.sval;
  }
}

Optional<ArrayRef<uint8_t>> DWARFFormValue::getAsBlock() const {
  if (!isFormClass(FC_Block) && !isFormClass(FC_Exprloc) &&
      Form != DW_FORM_data16)
    return None;
  return makeArrayRef(Value.data, Value.uval);
}

Optional<uint64_t> DWARFFormValue::getAsCStringOffset() const {
  if (!isFormClass(FC_String) && Form == DW_FORM_string)
    return None;
  return Value.uval;
}

Optional<uint64_t> DWARFFormValue::getAsReferenceUVal() const {
  if (!isFormClass(FC_Reference))
    return None;
  return Value.uval;
}
