///////////////////////////////////////////////////////////////////////////////
//
// CTypeInfoText.h 
//
// Author: Oleg Starodumov
// 
// Declaration of CTypeInfoText class 
//
//


#ifndef CTypeInfoText_h
#define CTypeInfoText_h


///////////////////////////////////////////////////////////////////////////////
// Include files 
//

#include "TypeInfoStructs.h"

#include "CTypeInfoDump.h"


///////////////////////////////////////////////////////////////////////////////
// CTypeInfoText class 
// 
// This purpose of this class is to provide textual information about 
// type information data. 
// 
// The most notable function is GetTypeName, which allows to obtain 
// the type identified by an index. 
// 
// Various data-to-string conversion functions provide default conversions 
// from various type information data and constants to strings. 
// Derived classes can override these functions to provide conversions 
// as desired by the user. 
//

class CTypeInfoText 
{
public: 

	// Constructors / destructor 

	CTypeInfoText( CTypeInfoDump* pDump ); 
	~CTypeInfoText();


	// Operations 

		// Get type name 
		// 
		// Parameters: 
		//   * [in]  Index:     Index of the type (whose name should be obtained) 
		//   * [in]  pVarName:  Name of the variable whose type it is 
		//                      (required only for functions and arrays) 
		//   * [out] pTypeName: Pointer to the buffer that receives the type name 
		//   * [out] MaxChars:  The maximal number of characters the buffer can store 
		// 
		// Return value: "true" if succeeded, "false" if failed 
		// 
	bool GetTypeName( ULONG Index,
	                  const TCHAR* pVarName,
	                  TCHAR* pTypeName,
	                  int MaxChars
	                 );


	// Conversions of various data to string

		// These functions provide the default implementation
		// for the data-to-string conversion.
		// It is possible to override them to provide user-defined conversions.
		//

	virtual LPCTSTR TagStr(      enum SymTagEnum Tag );
	virtual LPCTSTR BaseTypeStr( enum BasicType Type, ULONG64 Length = 0 );
	virtual LPCTSTR CallConvStr( enum CV_call_e CallConv );
	virtual LPCTSTR DataKindStr( SYMBOL_INFO& rSymbol );
	virtual LPCTSTR DataKindStr( enum DataKind dataKind );
	virtual void    SymbolLocationStr(  SYMBOL_INFO& rSymbol, TCHAR* pBuffer );
	virtual LPCTSTR UdtKindStr( enum UdtKind Kind );
	virtual LPCTSTR LocationTypeStr( enum LocationType Loc );


	// Accessors 

		// CTypeInfoDump object 
	CTypeInfoDump* DumpObj() 
		{ return m_pDump; }


	// Other functions 

		// This function is needed only to support TypeInfoDump demo application 
	virtual void ReportUdt( ULONG Index ) {}


private: 

	// Copy protection 
	CTypeInfoText( const CTypeInfoText& ); 
	CTypeInfoText& operator=( const CTypeInfoText& ); 

protected: 

	// Data members 

		// Pointer to CTypeInfoDump object 
	CTypeInfoDump* m_pDump; 

};


#endif // CTypeInfoText_h

