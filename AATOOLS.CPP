#include "stdafx.h"
#include "aastdinc.h"

/* ************************************************************************* */

AAHitProbDist::AAHitProbDist()
{
	m_nLowHitCount = 0;
	m_nHighHitCount = 0;
	pm_ProbArray = new double;
	pm_ProbArray[0] = 1.0;
}

AAHitProbDist::AAHitProbDist(const double *pPAry, int nLow, int nHigh)
{
	ASSERT(nHigh >= nLow);

	pm_ProbArray = new double[nHigh - nLow + 1];
	ASSERT(pm_ProbArray != NULL);

	for (int i = 0; i <= nHigh - nLow; i++)
		pm_ProbArray[i] = pPAry[i];

	m_nLowHitCount = nLow;
	m_nHighHitCount = nHigh;
}

AAHitProbDist::AAHitProbDist(const AAHitProbDist& aaPD)
{
	pm_ProbArray = new double[aaPD.m_nHighHitCount - aaPD.m_nLowHitCount + 1];
	ASSERT(pm_ProbArray != NULL);

	for (int i = 0; i <= aaPD.m_nHighHitCount - aaPD.m_nLowHitCount; i++)
		pm_ProbArray[i] = aaPD.pm_ProbArray[i];

	m_nLowHitCount = aaPD.m_nLowHitCount;
	m_nHighHitCount = aaPD.m_nHighHitCount;
}

AAHitProbDist& AAHitProbDist::operator=(const AAHitProbDist& aaPD)
{
	delete[] pm_ProbArray;
	pm_ProbArray = new double[aaPD.m_nHighHitCount - aaPD.m_nLowHitCount + 1];
	ASSERT(pm_ProbArray != NULL);

	for (int i = 0; i <= aaPD.m_nHighHitCount - aaPD.m_nLowHitCount; i++)
		pm_ProbArray[i] = aaPD.pm_ProbArray[i];

	m_nLowHitCount = aaPD.m_nLowHitCount;
	m_nHighHitCount = aaPD.m_nHighHitCount;

	return (*this);
}

AAHitProbDist AAHitProbDist::operator+(const AAHitProbDist& aaPD) const
{
	double *pSum;

	int nLow = m_nLowHitCount + aaPD.m_nLowHitCount;
	int nHigh = m_nHighHitCount + aaPD.m_nHighHitCount;
	
	pSum = new double[nHigh - nLow + 1];
	ASSERT(pSum != NULL);

	for (int i = nLow; i <= nHigh; i++)
		pSum[i] = 0.0;

	int j;
	for (i = m_nLowHitCount; i <= m_nHighHitCount; i++){
		for (j = aaPD.m_nLowHitCount; j <= aaPD.m_nHighHitCount; j++)
			pSum[i + j] += pm_ProbArray[i] * aaPD.pm_ProbArray[j];
	}

	AAHitProbDist retval(pSum, nLow, nHigh);
	delete[] pSum;
	return retval;
}	
	
#ifdef _DEBUG
void AAHitProbDist::debugDisplay()
{
	for (int i = (int)m_nLowHitCount; i <= (int)m_nHighHitCount; i++)
		TRACE("%f ", pm_ProbArray[i]);
}
#endif

/* ************************************************************************* */

AAUnitPowers::AAUnitPowers(AACombatPosition aaPos, AATechnologies aaTech)
{
	int naPowers[aaNUM_UNIT_TYPES];
	
	if (aaPos == aaPOS_ATT){
		naPowers[aaINFANTRY] = aaPOWER_ATT_INFANTRY;
		naPowers[aaARMOR] = aaPOWER_ATT_ARMOR;
		naPowers[aaFIGHTER] = aaPOWER_ATT_FIGHTER;
		naPowers[aaBOMBER] = aaPOWER_ATT_BOMBER;
		naPowers[aaTRANSPORT] = aaPOWER_ATT_TRANSPORT;
		naPowers[aaCARRIER] = aaPOWER_ATT_CARRIER;
		naPowers[aaSUB] = (aaTech.bSuperSubs == aaTRUE) ? aaPOWER_ATT_SUB_SUPER : aaPOWER_ATT_SUB;
		naPowers[aaBATTLESHIP] = aaPOWER_ATT_BATTLESHIP;
	}
	else{
		naPowers[aaINFANTRY] = aaPOWER_DEF_INFANTRY;
		naPowers[aaARMOR] = aaPOWER_DEF_ARMOR;
		naPowers[aaFIGHTER] = (aaTech.bJetFighters == aaTRUE) ? aaPOWER_DEF_FIGHTER_JET : aaPOWER_DEF_FIGHTER;
		naPowers[aaBOMBER] = aaPOWER_DEF_BOMBER;
		naPowers[aaTRANSPORT] = aaPOWER_DEF_TRANSPORT;
		naPowers[aaCARRIER] = aaPOWER_DEF_CARRIER;
		naPowers[aaSUB] = aaPOWER_DEF_SUB;
		naPowers[aaBATTLESHIP] = aaPOWER_DEF_BATTLESHIP;
	}

	double dArr[2];

	for (int i = 0; i < aaNUM_UNIT_TYPES; i++){
		dArr[1] = (double)naPowers[i] / 6;
		dArr[0] = 1 - dArr[1];
		m_aahpdaInitUnitHPDs[i] = AAHitProbDist(dArr, 0, 1);
	}

	//Account for the presence of heavy bombers here.
	if (aaPos == aaPOS_ATT && aaTech.bHeavyBombers == aaTRUE){
		AAHitProbDist aahpdHBDist;
		for (i = 1; i <= 3; i++)
			aahpdHBDist = aahpdHBDist + m_aahpdaInitUnitHPDs[(int)aaBOMBER];
		m_aahpdaInitUnitHPDs[(int)aaBOMBER] = aahpdHBDist;
	} 
}

inline AAHitProbDist AAUnitPowers::aahpdGetSingleUnitDist(AAUnitType aautType)	const
{
	return m_aahpdaInitUnitHPDs[(int)aautType];
}


/* ************************************************************************* */

AAUnitsCounts::AAUnitsCounts()
{
	for (int i = 0; i < aaNUM_UNIT_TYPES; i++)
	{
		m_naUnits[i] = 0;
	}
	
	m_nTotalNumUnits = 0;
}

void AAUnitsCounts::addUnits(AAUnitType aautType, int nNumUnits)
{
	m_naUnits[(int)aautType] += nNumUnits;
	m_nTotalNumUnits += nNumUnits;
}

void AAUnitsCounts::removeUnits(AAUnitType aautType, int nNumUnits)
{
	if (m_naUnits[(int)aautType] >= nNumUnits){
		m_naUnits[(int)aautType] -= nNumUnits;
		m_nTotalNumUnits -= nNumUnits;
	}
	else{
		m_nTotalNumUnits -= m_naUnits[(int)aautType];
		m_naUnits[(int)aautType] = 0;
	}

	//@@@@ should we fail if we remove "too many" units, or just set to zero?
}

int AAUnitsCounts::nGetNumUnitsOfType(AAUnitType aautType) const
{
	return m_naUnits[(int)aautType];
}

AABool AAUnitsCounts::operator==(const AAUnitsCounts& aauc)
{
	for (int i = 0; i < aaNUM_UNIT_TYPES; i++)
		if (m_naUnits[i] != aauc.m_naUnits[i])
			return aaFALSE;
	return aaTRUE;
}

CString AAUnitsCounts::strDescribe() const
{
	char buf[20];
	char* buf2;	//@@@@ delete?
	int nCnt;
	AABool bEmpty = aaTRUE;
	CString strRes("");
	CString strCount;
	CString strTypes("iafbTCSB");
	for (int i = 0; i < aaNUM_UNIT_TYPES; i++){
		nCnt = m_naUnits[(AAUnitType)i];
		if (nCnt > 0){
			buf2 =  _itoa(nCnt, buf, 10);
			strCount = CString(buf);
			strRes = strRes + strCount.Left(3) + strTypes.Mid(i, 1) + CString(" ");
			bEmpty = aaFALSE;
		}
	}
	if (bEmpty == aaTRUE)
		strRes = CString("nothing");

	return strRes;
}

#ifdef _DEBUG
void AAUnitsCounts::debugDisplay()
{
	TRACE("%di, ", m_naUnits[aaINFANTRY]);
	TRACE("%da, ", m_naUnits[aaARMOR]);
	TRACE("%dT, ", m_naUnits[aaTRANSPORT]);
	TRACE("%dS, ", m_naUnits[aaSUB]);
	TRACE("%df, ", m_naUnits[aaFIGHTER]);
	TRACE("%db, ", m_naUnits[aaBOMBER]);
	TRACE("%dC, ", m_naUnits[aaCARRIER]);
	TRACE("%dB", m_naUnits[aaBATTLESHIP]);
}
#endif

/* ************************************************************************* */

AAUnitsGroup::AAUnitsGroup()
{
	m_aaLoc = aaLOC_LAND;
	m_aaTech.bSuperSubs = aaFALSE;
	m_aaTech.bHeavyBombers = aaFALSE;
	m_aaTech.bJetFighters = aaFALSE;
}

AAUnitsGroup::AAUnitsGroup(AALocation aaloc, AATechnologies aatech)
{	
	m_aaLoc = aaloc;
	m_aaTech = aatech;
}

AAUnitsGroup::AAUnitsGroup(const AAUnitsGroup& aaug)
{
	m_aaucCounts = aaug.m_aaucCounts;
	m_aaTech = aaug.m_aaTech;
	m_aaLoc = aaug.m_aaLoc;
	m_aahpdSubs = aaug.m_aahpdSubs;
	m_aahpdNonSubs = aaug.m_aahpdNonSubs;
	m_aahpdLand = aaug.m_aahpdLand;
} 

AAUnitsGroup& AAUnitsGroup::operator=(const AAUnitsGroup& aaug)
{
	m_aaucCounts = aaug.m_aaucCounts;
	m_aaTech = aaug.m_aaTech;
	m_aaLoc = aaug.m_aaLoc;
	m_aahpdSubs = aaug.m_aahpdSubs;
	m_aahpdNonSubs = aaug.m_aahpdNonSubs;
	m_aahpdLand = aaug.m_aahpdLand;

	return (*this);
}

void AAUnitsGroup::addUnits(AAUnitType aautType, int nNumUnits, const AAUnitPowers& aapow)
{
	//assert type fits location

	AAHitProbDist aahpdSingle;
	aahpdSingle = aapow.aahpdGetSingleUnitDist(aautType);

	int i;
	
	//where does heavy bombers get handled? In the single PDs from aapow?
	
	if (nNumUnits > 0){
	
		if (m_aaLoc == aaLOC_SEA){
			if (aautType == aaSUB){
				for (i = 0; i < nNumUnits; i++)
					m_aahpdSubs = m_aahpdSubs + aahpdSingle;
			}
			else{
				for (i = 0; i < nNumUnits; i++)
					m_aahpdNonSubs = m_aahpdNonSubs + aahpdSingle;
			}
		}
		else{
			if (aautType == aaBATTLESHIP){
				for (i = 0; i < nNumUnits; i++)
					m_aahpdNonSubs = m_aahpdNonSubs + aahpdSingle;
			}
			else{
				for (i = 0; i < nNumUnits; i++){
					m_aahpdNonSubs = m_aahpdNonSubs + aahpdSingle;
					m_aahpdLand = m_aahpdLand + aahpdSingle;
				}
			}
		}
	}
	
	m_aaucCounts.addUnits(aautType, nNumUnits);
}

#ifdef _DEBUG
void AAUnitsGroup::debugDisplay()
{
	m_aaucCounts.debugDisplay();
}
#endif

/* ************************************************************************* */

AALossPreference::AALossPreference()
{
	m_aautPrefs[0] = aaINFANTRY;
	m_aautPrefs[1] = aaARMOR;
	m_aautPrefs[2] = aaTRANSPORT;
	m_aautPrefs[3] = aaSUB;
	m_aautPrefs[4] = aaFIGHTER;
	m_aautPrefs[5] = aaBOMBER;
	m_aautPrefs[6] = aaCARRIER;
	m_aautPrefs[7] = aaBATTLESHIP;
}

inline void AALossPreference::setTypePref(AAUnitType aautType, int nPref)
{
	ASSERT(nPref < aaNUM_UNIT_TYPES);
	m_aautPrefs[nPref] = aautType;
}

inline AAUnitType AALossPreference::aautGetTypeFromPref(int nPref) const
{
	ASSERT(nPref < aaNUM_UNIT_TYPES);
	return m_aautPrefs[nPref];
}

/* ************************************************************************* */

AABool AAForce::bUnitFitsLocation(AAUnitType aaut) const
{
	switch (m_loc) {
	case aaLOC_LAND:
		if (aaut == aaINFANTRY || aaut == aaARMOR || aaut == aaFIGHTER || aaut == aaBOMBER)
			return aaTRUE;
		else
			return aaFALSE;
	case aaLOC_SEA:
		if (aaut != aaINFANTRY && aaut != aaARMOR)
			return aaTRUE;
		else
			return aaFALSE;
	default:
		ASSERT(0);	//there should never be a default catch
		return aaFALSE;
	}
}

AAUnitType AAForce::aautFindBestNormalUnit(AAUnitsGroup& aaugToExtend) const
{
	AAUnitType aautProposed;

	for (int i = aaNUM_UNIT_TYPES - 1; i >= 0; i--) {
		aautProposed = m_aalosspKillOrder.aautGetTypeFromPref(i);
		if ((aaugToExtend.nGetNumUnitsOfType(aautProposed) < m_aaucCounts.nGetNumUnitsOfType(aautProposed)) && (bUnitIsSpecial(aautProposed) == aaFALSE) && (bUnitFitsLocation(aautProposed) == aaTRUE))
			return aautProposed;
	}

	return aautFindBestUnit(aaugToExtend);
}

AABool AAForce::bUnitIsSpecial(AAUnitType aaut) const
{
	switch (m_specialcc) {
	case aaSPECIAL_NONE:
		return aaFALSE;
	case aaSPECIAL_AA:
		if (aaut == aaFIGHTER || aaut == aaBOMBER)
			return aaTRUE;
		else
			return aaFALSE;
	case aaSPECIAL_SUB:
		if (aaut == aaTRANSPORT || aaut == aaSUB || aaut == aaCARRIER || aaut == aaBATTLESHIP)
			return aaTRUE;
		else
			return aaFALSE;
	default:
		ASSERT(0);	//there should never be a default catch
		return aaFALSE;
	}
}

AAUnitType AAForce::aautFindBestUnit(AAUnitsGroup& aaugToExtend) const
{
	AAUnitType aautProposed;

	for (int i = aaNUM_UNIT_TYPES - 1; i >= 0; i--) {
		aautProposed = m_aalosspKillOrder.aautGetTypeFromPref(i);
		if ((aaugToExtend.nGetNumUnitsOfType(aautProposed) < m_aaucCounts.nGetNumUnitsOfType(aautProposed)) && (bUnitFitsLocation(aautProposed) == aaTRUE))
			return aautProposed;
	}

	//shouldn't ever reach here.
	ASSERT(0);
	return aautProposed;
}

AAForce::AAForce(const AAUnitsCounts& aaucCounts, const AALossPreference& aalossp, AALocation aaLoc, AACombatPosition aaPos, AATechnologies aaTech, AASpecialCombatCircumstance aascc)
{
	m_aaucCounts = aaucCounts;
	m_aalosspKillOrder = aalossp;
	m_specialcc = aascc;
	m_loc = aaLoc;
	m_pos = aaPos;
	m_tech = aaTech;
	m_powers = AAUnitPowers(aaPos, aaTech);

	int nStatesToAdd = 1;
	int nLastStart = 0;
	int nLastEnd = 0;
	int nNewEnd = 0;
	AAUnitType aautNext;

	AAUnitsCounts aauc1;
	AAUnitsCounts aauc2;

	int i;
	int j;
	
	//Find out how many *relevant* units we are dealing with.
	int nNumUnitsInBaseUG = 0;
	for (i = 0; i < aaNUM_UNIT_TYPES; i++){
		if (bUnitFitsLocation(AAUnitType(i)) == aaTRUE)
			nNumUnitsInBaseUG += m_aaucCounts.nGetNumUnitsOfType(AAUnitType(i));
	}

	//We'll build nNumUnitsInBaseUG "layers", each of which is an array.
	//At the end, we'll link them all together.
	transListElement** ppTLLayers = new transListElement* [nNumUnitsInBaseUG + 1];
	ASSERT(ppTLLayers != NULL);
	int* pnaLayerStarts = new int[nNumUnitsInBaseUG + 1];
	ASSERT(pnaLayerStarts != NULL);
	int* pnaLayerEnds = new int[nNumUnitsInBaseUG + 1];
	ASSERT(pnaLayerEnds != NULL);
	ppTLLayers[0] = new transListElement;
	ASSERT(ppTLLayers[0] != NULL);
	ppTLLayers[0]->aaug = AAUnitsGroup(m_loc, m_tech);
	ppTLLayers[0]->nNormalLink = 0;
	ppTLLayers[0]->nSpecialLink = 0;
	pnaLayerStarts[0] = 0;
	pnaLayerEnds[0] = 0;
	int nLastLayerSize = 0;
	
	//Build the list in "layers" which have a specified number of units in them.
	for (i = 1; i <= nNumUnitsInBaseUG; i++) {
		
		ASSERT(i > 0);
		ASSERT(i <= nNumUnitsInBaseUG);
		
		nLastLayerSize = pnaLayerEnds[i - 1] - pnaLayerStarts[i - 1] + 1;
		
		//Start our count of states in the new layer with the number in the old layer.
		nStatesToAdd = nLastEnd - nLastStart + 1;
		
		//Find the unit we would add if we could add any (special or not)
		//unit to the "best" UG from the previous layer.
		aautNext = aautFindBestUnit(ppTLLayers[i - 1][nLastLayerSize - 1].aaug);
//		aautNext = aautFindBestUnit(pm_TL[nLastEnd].aaug);

		//If the unit is special, increment the new layer state count.
		if (bUnitIsSpecial(aautNext) == aaTRUE)
			nStatesToAdd++;

		//If adding a normal and adding any unit to the first state of the last layer
		//result in the same state, we'll want to add one fewer state for this layer.
		aauc1 = ppTLLayers[i - 1][0].aaug.aaucGetUnitsCounts();
		aauc2 = ppTLLayers[i - 1][0].aaug.aaucGetUnitsCounts();
		aauc1.addUnits(aautFindBestNormalUnit(ppTLLayers[i - 1][0].aaug), 1);
		aauc2.addUnits(aautFindBestUnit(ppTLLayers[i - 1][0].aaug), 1);
//		aauc1 = pm_TL[nLastStart].aaug.aaucGetUnitsCounts();
//		aauc2 = pm_TL[nLastStart].aaug.aaucGetUnitsCounts();
//		aauc1.addUnits(aautFindBestNormalUnit(pm_TL[nLastStart].aaug), 1);
//		aauc2.addUnits(aautFindBestUnit(pm_TL[nLastStart].aaug), 1);
		if (aauc1 == aauc2)
			nStatesToAdd--;

		//Make sure we're adding at least one state, and that we're not trying to
		//add too many considering how many units have not yet been added.
		nStatesToAdd = (nStatesToAdd < (nNumUnitsInBaseUG - i + 1)) ? nStatesToAdd : (nNumUnitsInBaseUG - i + 1);
		nStatesToAdd = nStatesToAdd > 0 ? nStatesToAdd : 1;
		ASSERT((nStatesToAdd > 0) && (nStatesToAdd <= nNumUnitsInBaseUG));

		//Record the end of this new layer.
		nNewEnd = nLastEnd + nStatesToAdd;
		
		ppTLLayers[i] = new transListElement[nStatesToAdd];
		ASSERT(ppTLLayers[i] != NULL);
		pnaLayerEnds[i] = nNewEnd;
		pnaLayerStarts[i] = nLastEnd + 1;
		
		//If the unit above was special we want to add a new list element with
		//both links pointing back to the "best" UG from the previous layer.
		if (bUnitIsSpecial(aautNext) == aaTRUE) {
			ppTLLayers[i][nStatesToAdd - 1].aaug = ppTLLayers[i - 1][nLastLayerSize - 1].aaug;
			ppTLLayers[i][nStatesToAdd - 1].aaug.addUnits(aautNext, 1, m_powers);
			ppTLLayers[i][nStatesToAdd - 1].nSpecialLink = nLastEnd;
			ppTLLayers[i][nStatesToAdd - 1].nNormalLink = nLastEnd;
//			pm_TL[nNewEnd].aaug = pm_TL[nLastEnd].aaug;
//			pm_TL[nNewEnd].aaug.addUnits(aautNext, 1, m_powers);
//			pm_TL[nNewEnd].nSpecialLink = nLastEnd;
//			pm_TL[nNewEnd].nNormalLink = nLastEnd;
			nStatesToAdd--;
			ASSERT((nStatesToAdd >= 0) && (nStatesToAdd <= nNumUnitsInBaseUG));
		}

		//Build the bulk of the new layer by adding normal units if possible.
		if (nStatesToAdd > 0){
			
			int nLayerElement = nStatesToAdd - 1;
			int nLastLayerElement = nLastLayerSize - 1;
			
			for (j = nLastEnd + nStatesToAdd; j >= nLastEnd + 1; j--){
				ASSERT(j >= nStatesToAdd);
				ASSERT(nLayerElement >= 0);
				ASSERT(nLayerElement < nStatesToAdd);
				ASSERT(nLastLayerElement >= 0);
				ASSERT(nLayerElement < nLastLayerSize);
				aautNext = aautFindBestNormalUnit(ppTLLayers[i - 1][nLastLayerElement].aaug);
				
				ASSERT(nLayerElement >= 0);
				ASSERT(nLayerElement < nStatesToAdd);
				ASSERT(nLastLayerElement >= 0);
				ASSERT(nLastLayerElement < nLastLayerSize);
				ppTLLayers[i][nLayerElement].aaug = ppTLLayers[i - 1][nLastLayerElement].aaug;
				ppTLLayers[i][nLayerElement].aaug.addUnits(aautNext, 1, m_powers);
				ppTLLayers[i][nLayerElement].nNormalLink = j - nStatesToAdd;
				if ( (j - nStatesToAdd - 1) >= nLastStart )
					ppTLLayers[i][nLayerElement].nSpecialLink = j - nStatesToAdd - 1;
				else
					ppTLLayers[i][nLayerElement].nSpecialLink = j;
				nLayerElement--;
				nLastLayerElement--;
			}
/*			for (j = nLastEnd + nStatesToAdd; j >= nLastEnd + 1; j--){
				ASSERT(j >= nStatesToAdd);
				aautNext = aautFindBestNormalUnit(pm_TL[j - nStatesToAdd].aaug);
				pm_TL[j].aaug = pm_TL[j - nStatesToAdd].aaug;
				pm_TL[j].aaug.addUnits(aautNext, 1, m_powers);
				pm_TL[j].nNormalLink = j - nStatesToAdd;
				if ( (j - nStatesToAdd - 1) >= nLastStart )
					pm_TL[j].nSpecialLink = j - nStatesToAdd - 1;
				else
					pm_TL[j].nSpecialLink = j;
			}
*/
		}
		
		nLastStart = nLastEnd + 1;
		nLastEnd = nNewEnd;
	}	

	ASSERT(nLastStart == nLastEnd);
	m_nListSize = nLastEnd + 1;
	
	//Now we can fill in the keeper array and throw our other structure away.
	pm_TL = new transListElement[m_nListSize];
	ASSERT(pm_TL != NULL);
    
    pm_TL[0] = ppTLLayers[0][0];
    int nTLE = 1;
	for (i = 1; i <= nNumUnitsInBaseUG; i++) {
		for (j = 0; j <= pnaLayerEnds[i] - pnaLayerStarts[i]; j++) {
			ASSERT (nTLE < m_nListSize);
			pm_TL[nTLE] = ppTLLayers[i][j];
			nTLE++;
		}
		delete[] ppTLLayers[i];
	}
	
	delete ppTLLayers[0];
	delete[] ppTLLayers;
	delete[] pnaLayerEnds;
	delete[] pnaLayerStarts;
				
	//We need to add in the battleship PDs to every state that has only special
	//units hit.
	if (m_loc == aaLOC_LAND && m_aaucCounts.nGetNumUnitsOfType(aaBATTLESHIP) > 0){
		
		//Start at the list end--the starting state of the battle.
		int nCurPos = nLastEnd;
		pm_TL[nCurPos].aaug.addUnits(aaBATTLESHIP, m_aaucCounts.nGetNumUnitsOfType(aaBATTLESHIP), m_powers);
		int nProposedNextPos = pm_TL[nCurPos].nSpecialLink;
		
		//Now, add battleships to every state we can reach by hitting only
		//special units--that is, air units killed by AA fire in this case.
		while (nProposedNextPos != nCurPos){
			nCurPos = nProposedNextPos;
			pm_TL[nCurPos].aaug.addUnits(aaBATTLESHIP, m_aaucCounts.nGetNumUnitsOfType(aaBATTLESHIP), m_powers);
			nProposedNextPos = pm_TL[nCurPos].nSpecialLink;
		}
	} 
	
	

	//DEBUG CODE
	#ifdef _DEBUG	
	AAHitProbDist aahpd;
	for (i = 0; i < (int)m_nListSize; i++){
		TRACE("%d:   ", i);
		pm_TL[i].aaug.debugDisplay();
		TRACE("    ( %d, %d )\n", pm_TL[i].nNormalLink, pm_TL[i].nSpecialLink);
		aahpd = pm_TL[i].aaug.aahpdGetNonSubsHPD();
		aahpd.debugDisplay();
		TRACE("\n");
		aahpd = pm_TL[i].aaug.aahpdGetSubsHPD();
		aahpd.debugDisplay();
		TRACE("\n");
		aahpd = pm_TL[i].aaug.aahpdGetLandHPD();
		aahpd.debugDisplay();
		TRACE("\n\n");
	}
	TRACE("\n\n");
	#endif //_DEBUG
	
}

AAForce::~AAForce()
{
	delete[] pm_TL;
}

AAForce& AAForce::operator=(const AAForce& force)
{
	m_nListSize = force.m_nListSize;
	
	pm_TL = new transListElement[m_nListSize];
	ASSERT(pm_TL != NULL);

	for (int i = 0; i < m_nListSize; i++)
		pm_TL[i] = force.pm_TL[i];
		
	m_aaucCounts = force.m_aaucCounts;
	m_aalosspKillOrder = force.m_aalosspKillOrder;
	m_specialcc = force.m_specialcc;
	m_loc = force.m_loc;
	m_tech = force.m_tech;
	m_pos = force.m_pos;
	m_powers = force.m_powers;

	return (*this);
}	

/*
AAUnitsGroup AAForce::aaugGetUGForState(int nElement) const
{
	ASSERT(nElement <= m_nListSize);
	return pm_TL[nElement].aaug;
}

inline int AAForce::nGetNextStateOnSpecial(int nElement) const
{
	ASSERT(nElement <= m_nListSize);
	return pm_TL[nElement].nSpecialLink;
}

int AAForce::nGetNextStateOnNormal(int nElement) const
{
	ASSERT(nElement <= m_nListSize);
	return pm_TL[nElement].nNormalLink;
}
*/

/* ************************************************************************* */
