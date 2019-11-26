///////////////////////////////////////////////////////////////////////////////
//
// TypeInfoStructs.h 
//
// Author: Oleg Starodumov
// 
// Declaration of various type information structures 
//
//


#ifndef TypeInfoStructs_h
#define TypeInfoStructs_h


///////////////////////////////////////////////////////////////////////////////
// Include files 
//

#define _NO_CVCONST_H
#include <Windows.h>
#include <DbgHelp.h>


///////////////////////////////////////////////////////////////////////////////
// Types from Cvconst.h (DIA SDK) 
//

#ifdef _NO_CVCONST_H

// BasicType, originally from CVCONST.H in DIA SDK 
enum BasicType
{
    btNoType = 0,
    btVoid = 1,
    btChar = 2,
    btWChar = 3,
    btInt = 6,
    btUInt = 7,
    btFloat = 8,
    btBCD = 9,
    btBool = 10,
    btLong = 13,
    btULong = 14,
    btCurrency = 25,
    btDate = 26,
    btVariant = 27,
    btComplex = 28,
    btBit = 29,
    btBSTR = 30,
    btHresult = 31
};

// UDtKind, originally from CVCONST.H in DIA SDK 
enum UdtKind
{
    UdtStruct,
    UdtClass,
    UdtUnion
};

// CV_call_e, originally from CVCONST.H in DIA SDK 
typedef enum CV_call_e {
    CV_CALL_NEAR_C      = 0x00, // near right to left push, caller pops stack
    CV_CALL_FAR_C       = 0x01, // far right to left push, caller pops stack
    CV_CALL_NEAR_PASCAL = 0x02, // near left to right push, callee pops stack
    CV_CALL_FAR_PASCAL  = 0x03, // far left to right push, callee pops stack
    CV_CALL_NEAR_FAST   = 0x04, // near left to right push with regs, callee pops stack
    CV_CALL_FAR_FAST    = 0x05, // far left to right push with regs, callee pops stack
    CV_CALL_SKIPPED     = 0x06, // skipped (unused) call index
    CV_CALL_NEAR_STD    = 0x07, // near standard call
    CV_CALL_FAR_STD     = 0x08, // far standard call
    CV_CALL_NEAR_SYS    = 0x09, // near sys call
    CV_CALL_FAR_SYS     = 0x0a, // far sys call
    CV_CALL_THISCALL    = 0x0b, // this call (this passed in register)
    CV_CALL_MIPSCALL    = 0x0c, // Mips call
    CV_CALL_GENERIC     = 0x0d, // Generic call sequence
    CV_CALL_ALPHACALL   = 0x0e, // Alpha call
    CV_CALL_PPCCALL     = 0x0f, // PPC call
    CV_CALL_SHCALL      = 0x10, // Hitachi SuperH call
    CV_CALL_ARMCALL     = 0x11, // ARM call
    CV_CALL_AM33CALL    = 0x12, // AM33 call
    CV_CALL_TRICALL     = 0x13, // TriCore Call
    CV_CALL_SH5CALL     = 0x14, // Hitachi SuperH-5 call
    CV_CALL_M32RCALL    = 0x15, // M32R Call
    CV_CALL_RESERVED    = 0x16  // first unused call enumeration
} CV_call_e;

// DataKind, originally from CVCONST.H in DIA SDK 
enum DataKind
{
    DataIsUnknown,
    DataIsLocal,
    DataIsStaticLocal,
    DataIsParam,
    DataIsObjectPtr,
    DataIsFileStatic,
    DataIsGlobal,
    DataIsMember,
    DataIsStaticMember,
    DataIsConstant
};

// CV_HREG_e, originally from CVCONST.H in DIA SDK 
typedef enum CV_HREG_e {

	// Only a limited number of registers included here 

    CV_REG_EAX      =  17,
    CV_REG_ECX      =  18,
    CV_REG_EDX      =  19,
    CV_REG_EBX      =  20,
    CV_REG_ESP      =  21,
    CV_REG_EBP      =  22,
    CV_REG_ESI      =  23,
    CV_REG_EDI      =  24,

} CV_HREG_e;

// LocatonType, originally from CVCONST.H in DIA SDK 
enum LocationType
{
    LocIsNull,
    LocIsStatic,
    LocIsTLS,
    LocIsRegRel,
    LocIsThisRel,
    LocIsEnregistered,
    LocIsBitField,
    LocIsSlot,
    LocIsIlRel,
    LocInMetaData,
    LocIsConstant,
    LocTypeMax
};

#endif // _NO_CVCONST_H


///////////////////////////////////////////////////////////////////////////////
// Constants 
//

// Maximal length of name buffers (in characters) 
#define TIS_MAXNAMELEN  256

// Maximal number of children (member variables, member functions, base classes, etc.) 
#define TIS_MAXNUMCHILDREN  64

// Maximal number of dimensions of an array 
#define TIS_MAXARRAYDIMS    64


///////////////////////////////////////////////////////////////////////////////
// Type information for various symbol kinds 
// 

// SymTagBaseType symbol 
struct BaseTypeInfo 
{
	// Basic type (DIA: baseType) 
	enum BasicType BaseType; 

	// Length (in bytes) (DIA: length)
	ULONG64 Length; 

};

// SymTagTypedef symbol 
struct TypedefInfo 
{
	// Name (DIA: name) 
	WCHAR Name[TIS_MAXNAMELEN]; 

	// Index of the underlying type (DIA: typeId) 
	ULONG TypeIndex; 

};

// SymTagPointerType symbol 
struct PointerTypeInfo 
{
	// Length (in bytes) (DIA: length) 
	ULONG64 Length; 

	// Index of the type the pointer points to (DIA: typeId) 
	ULONG TypeIndex; 

};

// SymTagUDT symbol (Class or structure) 
struct UdtClassInfo 
{
	// Name (DIA: name) 
	WCHAR Name[TIS_MAXNAMELEN]; 

	// Length (DIA: length) 
	ULONG64 Length; 

	// UDT kind (class, structure or union) (DIA: udtKind) 
	enum UdtKind UDTKind; 

	// Nested ("true" if the declaration is nested in another UDT) (DIA: nested) 
	bool Nested; 

	// Number of member variables 
	int NumVariables; 

	// Member variables 
	ULONG Variables[TIS_MAXNUMCHILDREN]; 

	// Number of member functions 
	int NumFunctions; 

	// Member functions 
	ULONG Functions[TIS_MAXNUMCHILDREN]; 

	// Number of base classes 
	int NumBaseClasses; 

	// Base classes 
	ULONG BaseClasses[TIS_MAXNUMCHILDREN]; 

};

// SymTagUDT symbol (Union) 
struct UdtUnionInfo 
{
	// Name (DIA: name) 
	WCHAR Name[TIS_MAXNAMELEN]; 

	// Length (in bytes) (DIA: length) 
	ULONG64 Length; 

	// UDT kind (class, structure or union) (DIA: udtKind) 
	enum UdtKind UDTKind; 

	// Nested ("true" if the declaration is nested in another UDT) (DIA: nested) 
	bool Nested; 

	// Number of members 
	int NumMembers; 

	// Members 
	ULONG Members[TIS_MAXNUMCHILDREN]; 

}; 

// SymTagBaseClass symbol 
struct BaseClassInfo 
{
	// Index of the UDT symbol that represents the base class (DIA: type) 
	ULONG TypeIndex; 

	// Virtual ("true" if the base class is a virtual base class) (DIA: virtualBaseClass) 
	bool Virtual; 

	// Offset of the base class within the class/structure (DIA: offset) 
	// (defined only if Virtual is "false")
	LONG Offset; 

	// Virtual base pointer offset (DIA: virtualBasePointerOffset) 
	// (defined only if Virtual is "true")
	LONG VirtualBasePointerOffset; 

};

// SymTagEnum symbol 
struct EnumInfo 
{
	// Name (DIA: name) 
	WCHAR Name[TIS_MAXNAMELEN]; 

	// Index of the symbol that represent the type of the enumerators (DIA: typeId) 
	ULONG TypeIndex; 

	// Nested ("true" if the declaration is nested in a UDT) (DIA: nested) 
	bool Nested; 

	// Number of enumerators 
	int NumEnums; 

	// Enumerators (their type indices) 
	ULONG Enums[TIS_MAXNUMCHILDREN]; 

}; 

// SymTagArrayType symbol 
struct ArrayTypeInfo 
{
	// Index of the symbol that represents the type of the array element 
	ULONG ElementTypeIndex; 

	// Index of the symbol that represents the type of the array index (DIA: arrayIndexTypeId) 
	ULONG IndexTypeIndex; 

	// Size of the array (in bytes) (DIA: length) 
	ULONG64 Length; 

	// Number of dimensions 
	int NumDimensions; 

	// Dimensions 
	ULONG64 Dimensions[TIS_MAXARRAYDIMS]; 

}; 

// SymTagFunctionType 
struct FunctionTypeInfo 
{
	// Index of the return value type symbol (DIA: objectPointerType)
	ULONG RetTypeIndex; 

	// Number of arguments (DIA: count) 
	int NumArgs; 

	// Function arguments 
	ULONG Args[TIS_MAXNUMCHILDREN]; 

	// Calling convention (DIA: callingConvention)
	enum CV_call_e CallConv; 

	// "Is member function" flag (member function, if "true") 
	bool MemberFunction; 

	// Class symbol index (DIA: classParent) 
	// (defined only if MemberFunction is "true") 
	ULONG ClassIndex; 

	// "this" adjustment (DIA: thisAdjust) 
	// (defined only if MemberFunction is "true") 
	LONG ThisAdjust; 

	// "Is static function" flag (static, if "true") 
	// (defined only if MemberFunction is "true") 
	bool StaticFunction; 

}; 

// SymTagFunctionArgType 
struct FunctionArgTypeInfo 
{
	// Index of the symbol that represents the type of the argument (DIA: typeId)
	ULONG TypeIndex; 
};

// SymTagData 
struct DataInfo 
{
	// Name (DIA: name) 
	WCHAR Name[TIS_MAXNAMELEN]; 

	// Index of the symbol that represents the type of the variable (DIA: type) 
	ULONG TypeIndex; 

	// Data kind (local, global, member, etc.) (DIA: dataKind) 
	enum DataKind dataKind; 

	// Address (defined if dataKind is: DataIsGlobal, DataIsStaticLocal, 
	// DataIsFileStatic, DataIsStaticMember) (DIA: address) 
	ULONG64 Address; 

	// Offset (defined if dataKind is: DataIsLocal, DataIsParam, 
	// DataIsObjectPtr, DataIsMember) (DIA: offset) 
	ULONG Offset; // <OS-TODO> Verify it for all listed data kinds 

	// Note: Length is not available - use the type symbol to obtain it 

};


///////////////////////////////////////////////////////////////////////////////
// TypeInfo structure (it contains all possible type information structures) 
//

struct TypeInfo 
{
	// Symbol tag 
	enum SymTagEnum Tag; 

	// UDT kind (defined only if "Tag" is SymTagUDT: "true" if the symbol is 
	// a class or a structure, "false" if the symbol is a union) 
	bool UdtKind; 

	// Union of all type information structures 
	union TypeInfoStructures 
	{
		BaseTypeInfo         sBaseTypeInfo;     // If Tag == SymTagBaseType 
		TypedefInfo          sTypedefInfo;      // If Tag == SymTagTypedef 
		PointerTypeInfo      sPointerTypeInfo;  // If Tag == SymTagPointerType 
		UdtClassInfo         sUdtClassInfo;     // If Tag == SymTagUDT and UdtKind is "true" 
		UdtUnionInfo         sUdtUnionInfo;     // If Tag == SymTagUDT and UdtKind is "false" 
		BaseClassInfo        sBaseClassInfo;    // If Tag == SymTagBaseClass 
		EnumInfo             sEnumInfo;         // If Tag == SymTagEnum 
		ArrayTypeInfo        sArrayTypeInfo;    // If Tag == SymTagArrayType 
		FunctionTypeInfo     sFunctionTypeInfo; // If Tag == SymTagFunctionType 
		FunctionArgTypeInfo  sFunctionArgTypeInfo; // If Tag == SymTagFunctionArgType 
		DataInfo             sDataInfo;         // If Tag == SymTagData 
	} Info;

}; 


#endif // TypeInfoStructs_h

