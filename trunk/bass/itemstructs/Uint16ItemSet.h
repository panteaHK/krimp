//
// Copyright (c) 2005-2012, Matthijs van Leeuwen, Jilles Vreeken, Koen Smets
// All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
// 
// Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
// Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials // provided with the distribution.
// Neither the name of the Universiteit Utrecht, Universiteit Antwerpen, nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
// 
#ifndef __UINT16ITEMSET_H
#define __UINT16ITEMSET_H

#include "ItemSet.h"

class BASS_API Uint16ItemSet : public ItemSet {
public:
	// Copy* Constructor
	Uint16ItemSet(const Uint16ItemSet* iset);
	// Create empty Uint16ItemSet
	Uint16ItemSet(uint32 cnt=1, uint32 freq=1, uint32 numOcc=0, double stdLen=0, uint32 *occurrences=NULL);
	// Create singleton Uint16ItemSet
	Uint16ItemSet(uint16 elem, uint32 cnt=1, uint32 freq=1, uint32 numOcc=0, double stdLen=0, uint32 *occurrences=NULL);
	// Create Uint16ItemSet
	Uint16ItemSet(const uint16 *set, uint32 setlen, uint32 cnt=1, uint32 freq=1, uint32 numOcc=0, double stdLen=0, uint32 *occurrences=NULL);

	virtual ~Uint16ItemSet();
	
	virtual Uint16ItemSet*	Clone() const;

	virtual uint32		GetLength() { return mLength; }

	// Memory-Usage estimator for this Uint16ItemSet
	virtual uint32		GetMemUsage() const ;
	// Generic Memory-Usage estimator for an Uint16ItemSet for a set of length `setLen`
	static uint32		MemUsage(const uint32 setlen);

	// Enumerator should be used in conjunction with for-loop over Length
	//virtual void		ResetEnumerator();
	//virtual uint16		GetNext();

	virtual void		RemoveDoubleItems(); // Uint16 only!

	virtual void		AddItemToSet(uint16 val);
	virtual void		RemoveItemFromSet(uint16 val);
	virtual bool		IsItemInSet(uint16 val) const;
	// Returns the last item in the set. Only call if length>0, as we do not check that here.
	virtual const uint16	GetLastItem() const { return (mLength > 0 ? mSet[mLength-1] : UINT16_MAX_VALUE); };

	// Fills the set with 0's from [o,max>
	virtual void		FillHerUp(uint16 max);
	virtual void		CleanTheSlate();

	virtual bool		Equals(ItemSet *is) const;
	virtual bool		Intersects(ItemSet *is) const;
	virtual bool		IsSubset(ItemSet *is) const;

	virtual void		Remove(ItemSet *is);

	virtual ItemSet*	Union(ItemSet *is) const;
	virtual ItemSet*	Intersection(ItemSet *is) const;

	virtual void		Unite(ItemSet *is);

	virtual void		Sort();

	// Translate from Fimi to Fic
	virtual void		TranslateForward(ItemTranslator *it);
	// Translate from Fic to Fimi
	virtual void		TranslateBackward(ItemTranslator *it);

	// returns ownership of an uint16 array with therein the values of the item set
	virtual uint16*		GetValues() const;
	// returns values of the item set in the provided uint16 values array
	virtual void		GetValuesIn(uint16 *vals) const;
	// returns values of the item set in the provided uint32 values array
	virtual void		GetValuesIn(uint32* vals) const;
	// increment the values of the item set in the provided uint32 values array
	virtual void		IncrementValues(uint32* vals, int32 cnt) const;

	virtual ItemSetType GetType() const { return Uint16ItemSetType; }

	// needed by Uint16CoverSet
	uint16*				GetSet() const { return mSet; }
	void				SetSet(uint16* set, uint32 len) { mSet = set; mLength = len; }

	virtual string		ToString(bool printCount=true, bool printFreq=false);
	virtual string		ToCodeTableString();

	// Compare item sets on alphabetical order. Returns 0 iff equal, -1 iff this<is, 1 iff this>is.
	virtual int8		CmpLexicographically(ItemSet *b);

protected:
	void				SetLength(uint16 length) { mLength = length; }

	void				QSortAscending(int first, int last);
	__inline void		SwapItems(int a, int b) { uint16 t = mSet[a]; mSet[a] = mSet[b]; mSet[b] = t; }

	uint16	*mSet;
	uint32	mLength;

	friend class Uint16CoverSet;
};

#endif // __UINT16ITEMSET_H
