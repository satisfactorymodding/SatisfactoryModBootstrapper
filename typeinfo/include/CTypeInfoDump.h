///////////////////////////////////////////////////////////////////////////////
//
// CTypeInfoDump.h 
//
// Author: Oleg Starodumov
// 
// Declaration of CTypeInfoDump class 
//
//


#ifndef CTypeInfoDump_h
#define CTypeInfoDump_h


///////////////////////////////////////////////////////////////////////////////
// Include files 
//

#include "TypeInfoStructs.h"


///////////////////////////////////////////////////////////////////////////////
// CTypeInfoDump class  
//

class CTypeInfoDump 
{

public: 

	// Constructors 

	CTypeInfoDump(); 
	CTypeInfoDump( HANDLE hProcess, DWORD64 ModBase ); 


	// Data accessors 

	HANDLE GetProcess() const 
		{ return m_hProcess; }
	DWORD64 GetModuleBase() const 
		{ return m_ModBase; }


	// Generic type information functions 

		// Dump a type 
	bool DumpType( ULONG Index,  TypeInfo& Info ); 

		// Dump type of a symbol 
	bool DumpSymbolType( ULONG Index, TypeInfo& Info, ULONG& TypeIndex ); 


	// Type - specific functions 

	bool DumpBasicType( ULONG Index, BaseTypeInfo& Info ); 
	bool DumpPointerType( ULONG Index, PointerTypeInfo& Info ); 
	bool DumpTypedef( ULONG Index, TypedefInfo& Info ); 
	bool DumpEnum( ULONG Index, EnumInfo& Info ); 
	bool DumpArrayType( ULONG Index, ArrayTypeInfo& Info ); 
	bool DumpUDT( ULONG Index, TypeInfo& Info ); 
	bool DumpUDTClass( ULONG Index, UdtClassInfo& Info ); 
	bool DumpUDTUnion( ULONG Index, UdtUnionInfo& Info ); 
	bool DumpFunctionType( ULONG Index, FunctionTypeInfo& Info ); 
	bool DumpFunctionArgType( ULONG Index, FunctionArgTypeInfo& Info ); 
	bool DumpBaseClass( ULONG Index, BaseClassInfo& Info ); 
	bool DumpData( ULONG Index, DataInfo& Info ); 


	// Helper functions 

		// Check if the symbol has the specified tag 
	bool CheckTag( ULONG Index, ULONG Tag ); 

		// Get size of the symbol 
	bool SymbolSize( ULONG Index, ULONG64& Size ); 

		// Get array element type index 
	bool ArrayElementTypeIndex( ULONG ArrayIndex, ULONG& ElementTypeIndex ); 

		// Get array dimensions 
	bool ArrayDims( ULONG Index, ULONG64* pDims, int& Dims, int MaxDims ); 

		// Get UDT member variables 
	bool UdtVariables( ULONG Index, ULONG* pVars, int& Vars, int MaxVars ); 

		// Get UDT member functions 
	bool UdtFunctions( ULONG Index, ULONG* pFuncs, int& Funcs, int MaxFuncs ); 

		// Get UDT base classes 
	bool UdtBaseClasses( ULONG Index, ULONG* pBases, int& Bases, int MaxBases ); 

		// Get members of a union 
	bool UdtUnionMembers( ULONG Index, ULONG* pMembers, int& Members, int MaxMembers ); 

		// Get function arguments 
	bool FunctionArguments( ULONG Index, ULONG* pArgs, int& Args, int MaxArgs ); 

		// Enumerators 
	bool Enumerators( ULONG Index, ULONG* pEnums, int& Enums, int MaxEnums ); 

		// Underlying type of a type definition 
	bool TypeDefType( ULONG Index, ULONG& UndTypeIndex ); 

		// Underlying type of a pointer type (the type the pointer points to)
	bool PointerType( ULONG Index, ULONG& UndTypeIndex, int& NumPointers ); 

private: 

	// Copy protection 
	CTypeInfoDump( const CTypeInfoDump& ); 
	CTypeInfoDump& operator=( const CTypeInfoDump& ); 

private: 

	// Data members 

		// Process handle 
	HANDLE m_hProcess; 

		// Module base address 
	DWORD64 m_ModBase; 

}; 


#endif // TypeInfoDump_h

