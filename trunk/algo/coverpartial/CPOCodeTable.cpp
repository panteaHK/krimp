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
#include "../../global.h"

#include <stdlib.h>

// -- qtils
#include <RandomUtils.h>

// -- bass
#include <db/Database.h>
#include <itemstructs/ItemSet.h>
#include <itemstructs/CoverSet.h>
#include <isc/ItemSetCollection.h>

#include "../CTISList.h"

#include "CPCodeTable.h"
#include "CPOCodeTable.h"

CPOCodeTable::CPOCodeTable() : CPCodeTable() {
	mCTList = NULL;
	mCT = NULL;
	mChangeType = ChangeNone;
	mOverlap = false;
	mUseOccs = NULL;
	mNumUseOccs = 0;
	mNumDBRows = 0;
}

CPOCodeTable::CPOCodeTable(const string &name) : CPCodeTable() {
	mCTList = NULL;
	mCT = NULL;
	mChangeType = ChangeNone;
	mUseOccs = NULL;
	mNumUseOccs = 0;
	mNumDBRows = 0;

	// coverpartial-orderly- stack random length etc
	if(name.compare(21,6,"random") == 0) {
		mOrder = CTRandomOrder;
		RandomUtils::Init();
	} else if(name.compare(21,6,"length") == 0) {
		if(name.compare(21,10,"length-inv") == 0)
			mOrder = CTLengthInverseOrder;
		else 
			mOrder = CTLengthOrder;
	} else if(name.compare(21,4,"area") == 0) {
		if(name.compare(21,8,"area-inv") == 0)
			mOrder = CTAreaInverseOrder;
		else 
			mOrder = CTAreaOrder;
	} else {
		if(name.compare(21,9,"stack-inv") == 0)
			mOrder = CTStackInverseOrder;
		else
			mOrder = CTStackOrder;
	}

	if(name.substr(name.length()-3,3).compare("-ov") == 0) {
		mOverlap = true;
		mCoverFunc = &CoverSet::CoverWithOverlap;
		mCanCoverFunc = &CoverSet::CanCoverWithOverlap;

		//mCoverFunc = &CoverSet::CoverWithOverlapNoSubsetCheck;
		//mCanCoverFunc = &CoverSet::CanCoverWithOverlapNoSubsetCheck;	// geen check nodig, want loop gaat toch alleen over dbOcc
	} else {
		mOverlap = false;
		mCoverFunc = &CoverSet::Cover;
		mCanCoverFunc = &CoverSet::CanCover;
	}
}

string CPOCodeTable::GetShortName() {
	string s = mOverlap ? "-ov" : "";
	if(mOrder == CTRandomOrder)
		return "cpor" + s;
	else if (mOrder == CTLengthOrder)
		return "cpol" + s;
	else if (mOrder == CTLengthInverseOrder)
		return "cpoli" + s;
	else if (mOrder == CTAreaOrder)
		return "cpoa" + s;
	else if (mOrder == CTAreaInverseOrder)
		return "cpoai" + s;
	else if (mOrder == CTStackInverseOrder) 
		return "cposi" + s;
	else /* if (mOrder == CTStackOrder) */
		return "cpos" + s;
}

CPOCodeTable::~CPOCodeTable() {
	if(mCTList != NULL) {
		islist::iterator cit, cend=mCTList->end();
		for(cit = mCTList->begin(); cit != cend; ++cit)
			delete *cit;
	}
	delete mCT;
	delete[] mUseOccs;
	mCTList = NULL;
}


void CPOCodeTable::UseThisStuff(Database *db, ItemSetType type, CTInitType ctinit, uint32 maxCTElemLength, uint32 toMinSup) {
	CPCodeTable::UseThisStuff(db, type, ctinit, maxCTElemLength, toMinSup);
	// 'CT in a List' stuffs
	mCT = new CTISList();
	mCTList = mCT->GetList();
	mCurNumSets = 0;

	mNumDBRows = mDB->GetNumRows();
	mUseOccs = new uint32[mNumDBRows];
	mIsBinnedDB = mDB->HasBinSizes();
}


// CPO Add :: Voegt `is` wel echt toe, maar set wel de mChange-vars ..
void CPOCodeTable::Add(ItemSet *is, uint64 candidateId) {
	if(mOrder == CTRandomOrder) {
		uint32 pos = RandomUtils::UniformUint32(mCurNumSets+1); // uint32 from [0,mCurNumSets]
		islist::iterator iter=mCTList->begin(), lend = mCTList->end();
		for(pos; pos>0 && iter!=lend; --pos)
			++iter;
		mChangeIter = mCTList->insert(iter,is);
	} else if(mOrder == CTLengthOrder) {
		islist::iterator iter, lend = mCTList->end();
		uint32 isl = is->GetLength();
		for(iter=mCTList->begin(); iter!=lend; ++iter) {
			if(isl > ((ItemSet*)(*iter))->GetLength())
				break;
		}
		mChangeIter = mCTList->insert(iter,is);
	} else if(mOrder == CTLengthInverseOrder) {
		islist::iterator iter, lend = mCTList->end();
		uint32 isl = is->GetLength();
		for(iter=mCTList->begin(); iter!=lend; ++iter) {
			if(isl < ((ItemSet*)(*iter))->GetLength())
				break;
		}
		mChangeIter = mCTList->insert(iter,is);
	} else if(mOrder == CTAreaOrder) {
		islist::iterator iter, lend = mCTList->end();
		uint32 area = is->GetLength() * is->GetSupport();
		for(iter=mCTList->begin(); iter!=lend; ++iter) {
			if(area > (((ItemSet*)(*iter))->GetLength() * ((ItemSet*)(*iter))->GetSupport()))
				break;
		}
		mChangeIter = mCTList->insert(iter,is);
	} else if(mOrder == CTAreaInverseOrder) {
		islist::iterator iter, lend = mCTList->end();
		uint32 area = is->GetLength() * is->GetSupport();
		for(iter=mCTList->begin(); iter!=lend; ++iter) {
			if(area < (((ItemSet*)(*iter))->GetLength() * ((ItemSet*)(*iter))->GetSupport()))
				break;
		}
		mChangeIter = mCTList->insert(iter,is);
	} else if(mOrder == CTStackInverseOrder) {
		mCTList->push_front(is);
		mChangeIter = mCTList->begin();
	} else if(mOrder == CTStackOrder) {
		mCTList->push_back(is);
		mChangeIter = mCTList->end();
		--mChangeIter;
		mCTList->begin();
	}

	++mCurNumSets;

	mChangeType = ChangeAdd;
	mChangeItem = is;
}

void CPOCodeTable::Del(ItemSet *is, bool zap, bool keepList) {
	mChangeType = ChangeDel;
	mChangeItem = is;

	mLastDel.is = is;
	if(mCTList->back() == is) {
		mChangeIter = mCTList->end();
		--mChangeIter;
		mLastDel.before = NULL;
	} else {
		islist::iterator iter, lend = mCTList->end();
		for(iter=mCTList->begin(); iter!=lend; ++iter)
			if((ItemSet*)(*iter) == is)
				break;
		mChangeIter = iter;
		lend = iter;
		++lend;
		mLastDel.before = (ItemSet*)(*lend);
	}

	if(keepList)
		mDeleted.push_front(DelInfo(mLastDel));

	--mCurNumSets;
	if(zap) {
		mCTList->erase(mChangeIter);
		delete is;
		mChangeType = ChangeNone;
		mChangeItem = NULL;
	}
}

// Rolls back last added ItemSet, restores counts and stats
void CPOCodeTable::RollbackAdd() {
	if(mChangeType == ChangeAdd && mChangeItem != NULL) {
		// Dan het ding van de aardbodem doen verdwijnen
		Del(mChangeItem, true, false);
		mLastDel.is = NULL;
		mCurStats = mPrevStats;
		// Eerst counts terugrollen, want dan is mRollback = mChangeIter nog geldig!
		RollbackCounts();
		mChangeType = ChangeNone;
		mChangeItem = NULL;
	}
}
void CPOCodeTable::RollbackLastDel() {
	if(mChangeType == ChangeDel && mLastDel.is != NULL)	{
		// Omdat er niet echt gedelete is, moeten alleen change en delinfo gezapt
		mChangeType = ChangeNone;
		mChangeItem = NULL;
		mChangeRow = -1;
		mCurStats = mPrevStats;
		RollbackCounts();

		++mCurNumSets;

		mLastDel.is = NULL;
	}
}

void CPOCodeTable::CommitLastDel(bool zap, bool updateLog) {
	if(mChangeType == ChangeDel) {
		if(updateLog)
			UpdateCTLog();

		mCTList->erase(mChangeIter);
		if(zap)
			delete mChangeItem;
		mLastDel.is = NULL;
		mPrevStats = mCurStats;
		mChangeType = ChangeNone;
		mChangeItem = NULL;
		mChangeRow = -1;
	}
}

void CPOCodeTable::CommitAllDels(bool updateLog) {
	THROW_DROP_SHIZZLE();
}
void CPOCodeTable::RollbackAllDels() {
	THROW_DROP_SHIZZLE();
}
void CPOCodeTable::RollbackCounts() {
	// Terugrollen van de counts moet -tot/met- de mRollback rij
	// aka, hier -tot- mRollback (die op list->end() of op mChangeIter ingesteld zal staan)
	for(islist::iterator cit = mCTList->begin(); cit != mRollback; ++cit)
		((ItemSet*)(*cit))->RollbackUsage();
	if(mRollbackAlph) {
		memcpy(mAlphabet, mOldAlphabet, mABLen * sizeof(uint32));
		mRollbackAlph = false;
	}
}

islist* CPOCodeTable::GetItemSetList() {
	islist *ilist = new islist();
	mCT->BuildItemSetList(ilist);
	return ilist;
}

islist* CPOCodeTable::GetPrePruneList() {
	islist *ilist = new	islist();
	mCT->BuildPrePruneList(ilist);
	return ilist;
}

void CPOCodeTable::UpdatePostPruneList(islist *pruneList, const islist::iterator &after) {
	SortPostPruneList(pruneList, after);
	mCT->BuildPostPruneList(pruneList, after);
}

void CPOCodeTable::CoverDB(CoverStats &stats) {
	if(mIsBinnedDB)			// switch binned/notbinned
		CoverBinnedDB(stats);
	else
		CoverNoBinsDB(stats);
}

void CPOCodeTable::CoverNoBinsDB(CoverStats &stats) {
	CoverSet *cset;
	ItemSet *iset;
	uint32 *occur, numOcc;
	islist::iterator cit,cend,naChangeIter;
	naChangeIter = mChangeIter;
	++naChangeIter;

	if(mChangeType == ChangeAdd) {
		// alle is' -tot/met- mChangeIter
		cend = naChangeIter;
		for(cit=mCTList->begin(); cit != cend; ++cit)
			((ItemSet*)(*cit))->BackupUsage();

		mChangeItem->SetPrevUsageCount(1);													// dus na ResetCount prevCount != 0
		occur = mChangeItem->GetDBOccurrence();
		numOcc = mChangeItem->GetNumDBOccurs();

		mNumUseOccs = 0;

		for(uint32 r=0; r<numOcc; r++) {
			uint32 i=occur[r];
			cset = mCDB[i];
			cset->ResetMask();

			// up till, but not including m
			cend = mChangeIter;															// up till, but not including m
			for(cit = mCTList->begin(); cit != cend; ++cit) {
				iset = *cit;
				if(iset->GetPrevUsageCount()==0)
					continue;
				(cset->*mCoverFunc)(iset->GetLength(),iset);
			}
			if((cset->*mCanCoverFunc)(mChangeItem->GetLength(),mChangeItem)) {			// nu m nog even doen
				mUseOccs[mNumUseOccs++] = i;
			}
		}
		if(mChangeItem!=NULL) {
			mChangeItem->SetPrevUsageCount(0);
			if(mNumUseOccs == 0) {
				mRollback = naChangeIter;	// ??? naChangeIter ivm zap?
				mRollbackAlph = false;
				return;
			}
		} 
		stats.usgCountSum += mNumUseOccs;
		mChangeItem->AddUsageCount(mNumUseOccs);
		// M heeft count > 0, dus we moeten de boel echt opnieuw uitrekenen...
		mRollback = mCTList->end();
		mRollbackAlph = true;
		// vanaf na mChangeIter counts up backen
		cend = mCTList->end();
		for(cit = naChangeIter; cit != cend; ++cit)
			((ItemSet*)(*cit))->BackupUsage();
		memcpy(mOldAlphabet, mAlphabet, mABLen * sizeof(uint32));

		uint32 i;
		for(uint32 r=0; r<mNumUseOccs; r++) {
			i=mUseOccs[r];
			cset = mCDB[i];

			cset->BackupMask();
			//	herstelcover (--) 
			// vanaf na mChangeIter counts herstellen
			for(cit = naChangeIter; cit != cend; ++cit)
				if((cset->*mCoverFunc)(((ItemSet*)(*cit))->GetLength(),*cit)) {
					((ItemSet*)(*cit))->SupOneFromUsageCount();
					stats.usgCountSum--;
				}
			stats.usgCountSum -= cset->UncoverAlphabet(mABLen, mAlphabet);

			//  Totale cover tot/met m
			cset->RestoreMask();
			(cset->*mCoverFunc)(mChangeItem->GetLength(),mChangeItem);

			//	Cover vanaf naChangeIter
			cend = mCTList->end();
			for(cit = naChangeIter; cit != cend; ++cit)
				//if(cset->Cover(((ItemSet*)(*cit))->GetLength(),*cit)) {
				if((cset->*mCoverFunc)(((ItemSet*)(*cit))->GetLength(),*cit)) {
					((ItemSet*)(*cit))->AddOneToUsageCount();
					stats.usgCountSum++;
				}
			stats.usgCountSum += cset->CoverAlphabet(mABLen, mAlphabet);
		}
		// Counts en CountSum zijn bijgewerkt, nu sizes uitrekenen ...

	} else if(mChangeType == ChangeDel) {
		uint32 st = 0;
		stats = mCurStats;
		
		mRollback = mCTList->end();
		mRollbackAlph = true;
		// vanaf naChangeIter de counts up backen
		cend = mCTList->end();
		for(cit = mCTList->begin(); cit != cend; ++cit)
			((ItemSet*)(*cit))->BackupUsage();
		memcpy(mOldAlphabet, mAlphabet, mABLen * sizeof(uint32));

		occur = mChangeItem->GetDBOccurrence();
		numOcc = mChangeItem->GetNumDBOccurs();

		stats.usgCountSum -= mChangeItem->GetPrevUsageCount();
		mChangeItem->SetUsageCount(0);	// want gedelete...
		uint32 ciLen = mChangeItem->GetLength();

		uint32 r,i;
		for(r=0; r<numOcc; r++) {
			i=occur[r];
			cset = mCDB[i];
			cset->ResetMask();

			// tot/met mChangeIter om cset op te bouwen
			for(cit = mCTList->begin(); cit != mChangeIter; ++cit) {
				iset = *cit;
				if(iset->GetPrevUsageCount()==0)
					continue;
				(cset->*mCoverFunc)(iset->GetLength(),iset);
			}
			if((cset->*mCanCoverFunc)(ciLen,mChangeItem)) {
				cset->BackupMask();
				(cset->*mCoverFunc)(ciLen,mChangeItem);

				// :: Herstelcover (--) 
				//	-: Herstel staartje mChangeRow
				cend = mCTList->end();
				for(cit = naChangeIter; cit != cend; ++cit) {
					iset = *cit;
					if((cset->*mCoverFunc)(iset->GetLength(),iset)) {
						iset->SupOneFromUsageCount();
						stats.usgCountSum--;
					}
				}
				//	-: Herstel alphabet counts
				stats.usgCountSum -= cset->UncoverAlphabet(mABLen, mAlphabet);

				cset->RestoreMask();
				// vanaf naChangeIter om cover zonder mChangeItem te bepalen
				for(cit = naChangeIter; cit != cend; ++cit) {
					if((cset->*mCoverFunc)(((ItemSet*)(*cit))->GetLength(),*cit)) {
						((ItemSet*)(*cit))->AddOneToUsageCount();
						stats.usgCountSum++;
					}
				}

				// alphabet items
				stats.usgCountSum += cset->CoverAlphabet(mABLen, mAlphabet);
			}
		}
	} else if(mChangeType == ChangeNone) {
		// init counts
		cend = mCTList->end();
		for(cit=mCTList->begin(); cit != cend; ++cit)
			((ItemSet*)(*cit))->ResetUsage();
		memcpy(mOldAlphabet, mAlphabet, mABLen * sizeof(uint32));
		for(uint32 i=0; i<mABUsedLen; i++)
			mAlphabet[i] = 0;

		stats.usgCountSum = mABUnusedCount;

		mRollback = mCTList->end();
		mRollbackAlph = true;

		// full cover db
		islist::iterator cit,cend;
		for(uint32 i=0; i<mNumDBRows; i++) {
			//mCoverSet->InitSet(mDB->GetRow(i));
			//mCDB[i]->InitSet(mDB->GetRow(i));
			mCDB[i]->ResetMask();

			cend = mCTList->end();
			for(cit=mCTList->begin(); cit != cend; ++cit) {
				if((mCDB[i]->*mCoverFunc)(((ItemSet*)(*cit))->GetLength(),*cit)) {
					((ItemSet*)(*cit))->AddOneToUsageCount();
					stats.usgCountSum++;
				}
			}

			// alphabet items
			stats.usgCountSum += mCDB[i]->CoverAlphabet(mABLen, mAlphabet);
		}
	} else
		THROW("Unknown code table ChangeType");

	// Recalc sizes, counts and countSum should be up to date...
	stats.encDbSize = 0;
	stats.encCTSize = 0;
	stats.numSetsUsed = 0;
	stats.alphItemsUsed = 0;

	double codeLen = 0;
	// alle is <- ct
	cend = mCTList->end();
	for(cit = mCTList->begin(); cit != cend; ++cit) {
		uint32 curcnt = ((ItemSet*)(*cit))->GetUsageCount();
		if(curcnt == 0) continue;
		stats.numSetsUsed++;
		codeLen = CalcCodeLength(curcnt, stats.usgCountSum);
		stats.encDbSize += curcnt * codeLen;
		stats.encCTSize += ((ItemSet*)(*cit))->GetStandardLength();
		stats.encCTSize += codeLen;
	}
		
	// Alphabet
	for(uint32 i=0; i<mABLen; i++) {
		uint32 curcnt = mAlphabet[i];
		if(curcnt == 0) continue;
		stats.alphItemsUsed++;
		codeLen = CalcCodeLength(curcnt, stats.usgCountSum);
		stats.encDbSize += curcnt * codeLen;
		stats.encCTSize += mStdLengths[i];
		stats.encCTSize += codeLen;
	}
	stats.encSize = stats.encDbSize + stats.encCTSize;
}

void CPOCodeTable::CoverBinnedDB(CoverStats &stats) {
	CoverSet *cset;
	ItemSet *iset;
	uint32 *occur, numOcc, numUseCount;
	islist::iterator cit,cend,naChangeIter;
	naChangeIter = mChangeIter;
	++naChangeIter;

	if(mChangeType == ChangeAdd) {
		// alle is' -tot/met- mChangeIter
		cend = naChangeIter;
		for(cit=mCTList->begin(); cit != cend; ++cit)
			((ItemSet*)(*cit))->BackupUsage();

		mChangeItem->SetPrevUsageCount(1);													// dus na ResetCount prevCount != 0
		occur = mChangeItem->GetDBOccurrence();
		numOcc = mChangeItem->GetNumDBOccurs();

		mNumUseOccs = 0;
		numUseCount = 0;

		for(uint32 r=0; r<numOcc; r++) {
			uint32 i=occur[r];
			cset = mCDB[i];
			cset->ResetMask();

			// up till, but not including m
			cend = mChangeIter;															// up till, but not including m
			for(cit = mCTList->begin(); cit != cend; ++cit) {
				iset = *cit;
				if(iset->GetPrevUsageCount()==0)
					continue;
				(cset->*mCoverFunc)(iset->GetLength(),iset);
			}
			if((cset->*mCanCoverFunc)(mChangeItem->GetLength(),mChangeItem)) {			// nu m nog even doen
				uint32 binSize = mDB->GetRow(i)->GetUsageCount();
				mUseOccs[mNumUseOccs++] = i;
				numUseCount += binSize;
			}
		}
		if(mChangeItem!=NULL) {
			mChangeItem->SetPrevUsageCount(0);
			if(mNumUseOccs == 0) {
				mRollback = naChangeIter;	// ??? naChangeIter ivm zap?
				mRollbackAlph = false;
				return;
			}
		} 
		stats.usgCountSum += numUseCount;
		mChangeItem->AddUsageCount(numUseCount);
		// M heeft count > 0, dus we moeten de boel echt opnieuw uitrekenen...
		mRollback = mCTList->end();
		mRollbackAlph = true;
		// vanaf na mChangeIter counts up backen
		cend = mCTList->end();
		for(cit = naChangeIter; cit != cend; ++cit)
			((ItemSet*)(*cit))->BackupUsage();
		memcpy(mOldAlphabet, mAlphabet, mABLen * sizeof(uint32));

		uint32 i;
		for(uint32 r=0; r<mNumUseOccs; r++) {
			i=mUseOccs[r];
			cset = mCDB[i];

			uint32 binSize = mDB->GetRow(i)->GetUsageCount();

			cset->BackupMask();
			//	herstelcover (--) 
			// vanaf na mChangeIter counts herstellen
			for(cit = naChangeIter; cit != cend; ++cit)
				
				if((cset->*mCoverFunc)(((ItemSet*)(*cit))->GetLength(),*cit)) {
					((ItemSet*)(*cit))->SupUsageCount(binSize);
					stats.usgCountSum -= binSize;
				}
				stats.usgCountSum -= cset->UncoverAlphabet(mABLen, mAlphabet, binSize);

				//  Totale cover tot/met m
				cset->RestoreMask();
				(cset->*mCoverFunc)(mChangeItem->GetLength(),mChangeItem);

				//	Cover vanaf naChangeIter
				cend = mCTList->end();
				for(cit = naChangeIter; cit != cend; ++cit)
					//if(cset->Cover(((ItemSet*)(*cit))->GetLength(),*cit)) {
					if((cset->*mCoverFunc)(((ItemSet*)(*cit))->GetLength(),*cit)) {
						((ItemSet*)(*cit))->AddUsageCount(binSize);
						stats.usgCountSum += binSize;
					}
					stats.usgCountSum += cset->CoverAlphabet(mABLen, mAlphabet, binSize);
		}
		// Counts en CountSum zijn bijgewerkt, nu sizes uitrekenen ...

	} else if(mChangeType == ChangeDel) {
		uint32 st = 0;
		stats = mCurStats;

		mRollback = mCTList->end();
		mRollbackAlph = true;
		// vanaf naChangeIter de counts up backen
		cend = mCTList->end();
		for(cit = mCTList->begin(); cit != cend; ++cit)
			((ItemSet*)(*cit))->BackupUsage();
		memcpy(mOldAlphabet, mAlphabet, mABLen * sizeof(uint32));

		occur = mChangeItem->GetDBOccurrence();
		numOcc = mChangeItem->GetNumDBOccurs();

		stats.usgCountSum -= mChangeItem->GetPrevUsageCount();
		mChangeItem->SetUsageCount(0);	// want gedelete...
		uint32 ciLen = mChangeItem->GetLength();

		uint32 r,i;
		for(r=0; r<numOcc; r++) {
			i=occur[r];
			cset = mCDB[i];
			uint32 binSize = mDB->GetRow(i)->GetUsageCount();
			cset->ResetMask();

			// tot/met mChangeIter om cset op te bouwen
			for(cit = mCTList->begin(); cit != mChangeIter; ++cit) {
				iset = *cit;
				if(iset->GetPrevUsageCount()==0)
					continue;
				(cset->*mCoverFunc)(iset->GetLength(),iset);
			}
			if((cset->*mCanCoverFunc)(ciLen,mChangeItem)) {
				cset->BackupMask();
				(cset->*mCoverFunc)(ciLen,mChangeItem);

				// :: Herstelcover (--) 
				//	-: Herstel staartje mChangeRow
				cend = mCTList->end();
				for(cit = naChangeIter; cit != cend; ++cit) {
					iset = *cit;
					if((cset->*mCoverFunc)(iset->GetLength(),iset)) {
						iset->SupUsageCount(binSize);
						stats.usgCountSum-=binSize;
					}
				}
				//	-: Herstel alphabet counts
				stats.usgCountSum -= cset->UncoverAlphabet(mABLen, mAlphabet,binSize);

				cset->RestoreMask();
				// vanaf naChangeIter om cover zonder mChangeItem te bepalen
				for(cit = naChangeIter; cit != cend; ++cit) {
					if((cset->*mCoverFunc)(((ItemSet*)(*cit))->GetLength(),*cit)) {
						((ItemSet*)(*cit))->AddUsageCount(binSize);
						stats.usgCountSum+=binSize;
					}
				}

				// alphabet items
				stats.usgCountSum += cset->CoverAlphabet(mABLen, mAlphabet, binSize);
			}
		}
	} else if(mChangeType == ChangeNone) {
		// init counts
		cend = mCTList->end();
		for(cit=mCTList->begin(); cit != cend; ++cit)
			((ItemSet*)(*cit))->ResetUsage();
		memcpy(mOldAlphabet, mAlphabet, mABLen * sizeof(uint32));
		for(uint32 i=0; i<mABUsedLen; i++)
			mAlphabet[i] = 0;

		stats.usgCountSum = mABUnusedCount;

		mRollback = mCTList->end();
		mRollbackAlph = true;

		// full cover db
		islist::iterator cit,cend;
		for(uint32 i=0; i<mNumDBRows; i++) {
			//mCoverSet->InitSet(mDB->GetRow(i));
			//mCDB[i]->InitSet(mDB->GetRow(i));
			mCDB[i]->ResetMask();
			uint32 binSize = mDB->GetRow(i)->GetUsageCount();

			cend = mCTList->end();
			for(cit=mCTList->begin(); cit != cend; ++cit) {
				if((mCDB[i]->*mCoverFunc)(((ItemSet*)(*cit))->GetLength(),*cit)) {
					((ItemSet*)(*cit))->AddUsageCount(binSize);
					stats.usgCountSum+=binSize;
				}
			}

			// alphabet items
			stats.usgCountSum += mCDB[i]->CoverAlphabet(mABLen, mAlphabet, binSize);
		}
	} else
		THROW("Unknown code table ChangeType");

	CalcTotalSize(stats);
}

void CPOCodeTable::CalcTotalSize(CoverStats &stats) {
	islist::iterator cit,cend;

	// Recalc sizes, counts and countSum should be up to date...
	stats.encDbSize = 0;
	stats.encCTSize = 0;
	stats.numSetsUsed = 0;
	stats.alphItemsUsed = 0;

	double codeLen = 0;
	// alle is <- ct
	cend = mCTList->end();
	for(cit = mCTList->begin(); cit != cend; ++cit) {
		uint32 curcnt = ((ItemSet*)(*cit))->GetUsageCount();
		if(curcnt == 0) continue;
		stats.numSetsUsed++;
		codeLen = CalcCodeLength(curcnt, stats.usgCountSum);
		stats.encDbSize += curcnt * codeLen;
		stats.encCTSize += ((ItemSet*)(*cit))->GetStandardLength();
		stats.encCTSize += codeLen;
	}

	// Alphabet
	for(uint32 i=0; i<mABLen; i++) {
		uint32 curcnt = mAlphabet[i];
		if(curcnt == 0) continue;
		stats.alphItemsUsed++;
		codeLen = CalcCodeLength(curcnt, stats.usgCountSum);
		stats.encDbSize += curcnt * codeLen;
		stats.encCTSize += mStdLengths[i];
		stats.encCTSize += codeLen;
	}
	stats.encSize = stats.encDbSize + stats.encCTSize;
}


double	CPOCodeTable::CalcTransactionCodeLength(ItemSet *transaction) {
	islist::iterator cit,cend = mCTList->end();;
	if(mCoverSet == NULL)
		mCoverSet = CoverSet::Create(mCoverSetType, mDB->GetAlphabetSize());
	mCoverSet->InitSet(transaction);

	double codeLen = 0.0;
	for(cit = mCTList->begin(); cit != cend; ++cit) {
		if(mCoverSet->Cover(((ItemSet*)(*cit))->GetLength(),*cit))
			codeLen += CalcCodeLength((*cit)->GetUsageCount(), mCurStats.usgCountSum);
	}
	// alphabet items
	for(uint32 i=0; i<mABLen; i++)
		if(mCoverSet->IsItemUncovered(i))
			codeLen += CalcCodeLength(mAlphabet[i], mCurStats.usgCountSum);
	return codeLen;
}

void	CPOCodeTable::CoverMaskedDB(CoverStats &stats, uint32 *rowList, uint32 numRows) {
	// init counts
	mCT->ResetCounts();
	memcpy(mOldAlphabet, mAlphabet, mABLen * sizeof(uint32));
	for(uint32 i=0; i<mABLen; i++)
		mAlphabet[i] = 0;

	stats.usgCountSum = 0;
	stats.numSetsUsed = 0;
	stats.alphItemsUsed = 0;
	stats.encDbSize = 0;
	stats.encCTSize = 0;
	stats.encSize = 0;

	// full cover db
	islist::iterator cit,cend = mCTList->end();
	for(uint32 i=0; i<numRows; i++) {
		mCoverSet->InitSet(mDB->GetRow(rowList[i]));

		for(cit = mCTList->begin(); cit != cend; ++cit) {
			if(mCoverSet->Cover(((ItemSet*)(*cit))->GetLength(),*cit)) {
				((ItemSet*)(*cit))->AddOneToUsageCount();
				stats.usgCountSum++;
			}
		}
		// alphabet items
		stats.usgCountSum += mCoverSet->CoverAlphabet(mABLen, mAlphabet);
	}

	double codeLen = 0;
	uint32 curcnt;
	for (cit = mCTList->begin(); cit != cend; ++cit) {
		curcnt = ((ItemSet*)(*cit))->GetUsageCount();
		if(curcnt == 0) continue;
		stats.numSetsUsed++;
		codeLen = CalcCodeLength(curcnt, stats.usgCountSum);
		stats.encDbSize += curcnt * codeLen;
		stats.encCTSize += ((ItemSet*)(*cit))->GetStandardLength() + codeLen;
	}
	// Alphabet
	for(uint32 i=0; i<mABLen; i++) {
		uint32 curcnt = mAlphabet[i];
		if(curcnt == 0) continue;
		stats.alphItemsUsed++;
		codeLen = CalcCodeLength(curcnt, stats.usgCountSum);
		stats.encDbSize += curcnt * codeLen;
		stats.encCTSize += mStdLengths[i] + codeLen;
	}

	stats.encSize = stats.encDbSize + stats.encCTSize;
}
Database *CPOCodeTable::EncodeDB(const bool useSingletons, string outFile) {
	ECHO(1,	printf(" * Encoding database with CPO...\r"));
	islist::iterator cit,cend;
	uint16 val=0;
	uint16 *emptySet = new uint16[0];
	ItemSet *set = NULL;

	outFile += ".encoded";
	FILE *fp = fopen(outFile.c_str(), "w");
	fprintf(fp, "algo=%s\n", GetShortName().c_str());

	ItemSet **iss = new ItemSet *[mNumDBRows];
	cend = mCTList->end();
	for(uint32 i=0; i<mNumDBRows; i++) {
		set = iss[i] = ItemSet::Create(Uint16ItemSetType, emptySet, 0, mCurNumSets);
		set->SetUsageCount(mDB->GetRow(i)->GetUsageCount());
		mCDB[i]->ResetMask();

		val = 0;
		// sets
		for(cit=mCTList->begin(); cit != cend; ++cit) {
			if((mCDB[i]->*mCoverFunc)(((ItemSet*)(*cit))->GetLength(),*cit)) {
				set->AddItemToSet(val);
				fprintf(fp, "{%s", (*cit)->ToString().c_str());
				fseek(fp, -2, SEEK_CUR);
				fprintf(fp, "/%d)} ", (*cit)->GetSupport());
			}
			val++;
		}

		// singletons
		if(useSingletons)
			for(uint32 j=0; j<mABLen; j++, val++)
				if(mCDB[i]->IsItemUncovered(j)) {
					set->AddItemToSet(val);
					fprintf(fp, "{%d} ", j);
				}
				fprintf(fp, "\n");
	}

	delete[] emptySet;
	fclose(fp);
	ECHO(1,	printf(" * Encoding:\t\tdone (used CPO)\n\n"));
	return new Database(iss, mNumDBRows, mDB->HasBinSizes());
}
void CPOCodeTable::UpdateCTLog() {
	if(mChangeType == ChangeAdd) {
		fprintf(mCTLogFile, "+ %s\n", mChangeItem->ToCodeTableString().c_str());
	} else if(mChangeType == ChangeDel) {
		fprintf(mCTLogFile, "- %s\n", mChangeItem->ToCodeTableString().c_str());
	} else if(mChangeType == ChangeNone) {
		fprintf(mCTLogFile, "0\n");
	}
}