/**************************************************************************
 * Copyright(c) 1998-1999, ALICE Experiment at CERN, All rights reserved. *
 *                                                                        *
 * Author: The ALICE Off-line Project.                                    *
 * Contributors are mentioned in the code where appropriate.              *
 *                                                                        *
 * Permission to use, copy, modify and distribute this software and its   *
 * documentation strictly for non-commercial purposes is hereby granted   *
 * without fee, provided that the above copyright notice appears in all   *
 * copies and that both the copyright notice and this permission notice   *
 * appear in the supporting documentation. The authors make no claims     *
 * about the suitability of this software for any purpose. It is          *
 * provided "as is" without express or implied warranty.                  *
 **************************************************************************/

/* $Id$ */

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// This class provides access to TRD digits in raw data.                     //
//                                                                           //
// It loops over all TRD digits in the raw data given by the AliRawReader.   //
// The Next method goes to the next digit. If there are no digits left       //
// it returns kFALSE.                                                        //
// Several getters provide information about the current digit.              //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include "AliLog.h"
#include "AliRawReader.h"

#include "AliTRDRawStream.h"
#include "AliTRDcalibDB.h"

ClassImp(AliTRDRawStream)

//_____________________________________________________________________________
AliTRDRawStream::AliTRDRawStream() 
  :TObject()
  ,fRawReader(0)
  ,fCount(0)
  ,fDetector(-1)
  ,fPrevDetector(-1)
  ,fNPads(-1)
  ,fRow(-1)
  ,fPrevRow(-1)
  ,fColumn(-1)
  ,fPrevColumn(-1)
  ,fTime(-1)
  ,fSignal(-1)
{
  //
  // Default constructor
  //

}

//_____________________________________________________________________________
AliTRDRawStream::AliTRDRawStream(AliRawReader* rawReader) 
  :TObject()
  ,fRawReader(rawReader)
  ,fCount(0)
  ,fDetector(-1)
  ,fPrevDetector(-1)
  ,fNPads(-1)
  ,fRow(-1)
  ,fPrevRow(-1)
  ,fColumn(-1)
  ,fPrevColumn(-1)
  ,fTime(-1)
  ,fSignal(-1)
{
  //
  // Create an object to read TRD raw digits
  //

  fRawReader->Select("TRD");

}

//_____________________________________________________________________________
AliTRDRawStream::AliTRDRawStream(const AliTRDRawStream& stream) :
  TObject(stream),
  fRawReader(NULL),
  fCount(0),
  fDetector(-1),
  fPrevDetector(-1),
  fNPads(-1),
  fRow(-1),
  fPrevRow(-1),
  fColumn(-1),
  fPrevColumn(-1),
  fTime(-1),
  fSignal(-1)
{
  //
  // Copy constructor
  //

  AliFatal("Copy constructor not implemented");

}

//_____________________________________________________________________________
AliTRDRawStream& AliTRDRawStream::operator = (const AliTRDRawStream& 
					      /* stream */)
{
  //
  // Assigment operator
  //

  Fatal("operator =", "assignment operator not implemented");
  return *this;

}

//_____________________________________________________________________________
AliTRDRawStream::~AliTRDRawStream()
{
  //
  // Destructor
  //

}

//_____________________________________________________________________________
Bool_t AliTRDRawStream::Next()
{
  //
  // Read the next raw digit
  // Returns kFALSE if there is no digit left
  //

  fPrevDetector = fDetector;
  fPrevRow      = fRow;
  fPrevColumn   = fColumn;
  UChar_t data;

  AliTRDcalibDB* calibration = AliTRDcalibDB::Instance();
  if (!calibration)
    return kFALSE;
  
  Int_t timeBins = calibration->GetNumberOfTimeBins();
  
  while (fCount >= 0) {

    while (fCount == 0) {  // next detector
      // read the flag
      if (!fRawReader->ReadNextChar(data)) return kFALSE;
      if (data != 0xBB) {
	AliError(Form("wrong flag: %x", data));
	fCount = -1;
	return kFALSE;
      }

      // read the detector number
      if (!fRawReader->ReadNextChar(data)) {
	AliError("Could not read detector number");
	fCount = -1;
	return kFALSE;
      }
      fDetector = data;
      if (!fRawReader->ReadNextChar(data)) {
	AliError("Could not read detector number");
	fCount = -1;
	return kFALSE;
      }
      fDetector += (UInt_t(data) << 8);

      // read the number of byts
      if (!fRawReader->ReadNextChar(data)) {
	AliError("Could not read number of bytes");
	fCount = -1;
	return kFALSE;
      }
      fCount = data;
      if (!fRawReader->ReadNextChar(data)) {
	AliError("Could not read number of bytes");
	fCount = -1;
	return kFALSE;
      }
      fCount += (UInt_t(data) << 8);
      if (!fRawReader->ReadNextChar(data)) {
        AliError("Could not read number of bytes");
        fCount = -1;
        return kFALSE;
      }
      fCount += (UInt_t(data) << 16);

      // read the number of active pads
      if (!fRawReader->ReadNextChar(data)) {
	AliError("Could not read number of active pads");
	fCount = -1;
	return kFALSE;
      }
      fNPads = data;
      if (!fRawReader->ReadNextChar(data)) {
	AliError("Could not read number of active pads");
	fCount = -1;
	return kFALSE;
      }
      fNPads += (UInt_t(data) << 8);

      fTime = timeBins;

    }

    // read the pad row and column number
    if ((fTime >= timeBins) && (fCount > 2)) {
      if (!fRawReader->ReadNextChar(data)) {
	AliError("Could not read row number");
	fCount = -1;
	return kFALSE;
      }
      fCount--;
      fRow = data - 1;
      if (!fRawReader->ReadNextChar(data)) {
	AliError("Could not read column number");
	fCount = -1;
	return kFALSE;
      }
      fCount--;
      fColumn = data - 1;
      fTime = 0;
    }

    // read the next data byte
    if (!fRawReader->ReadNextChar(data)) {
      AliError("Could not read data");
      fCount = -1;
      return kFALSE;
    }
    fCount--;

    if (data == 0) {  // zeros
      if (!fRawReader->ReadNextChar(data)) {
	AliError("Could not read time value");
	fCount = -1;
	return kFALSE;
      }
      fCount--;
      fTime += data + 1;

    } 
    else {          // signal
      fSignal = (UInt_t(data & 0x7F) << 8);
      if (!fRawReader->ReadNextChar(data)) {
	AliError("Could not read ADC value");
	fCount = -1;
	return kFALSE;
      }
      fCount--;
      fSignal += data;
      fTime++;
      return kTRUE;
    }
  }

  return kFALSE;

}
