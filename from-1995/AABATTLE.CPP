#include "stdafx.h"
#include "aastdinc.h"

/* ************************************************************************* */

AABattle::AABattle(const AASideInfo& aasideAtt, const AASideInfo& aasideDef, AALocation aaloc)
{
	m_aasideAtt = aasideAtt;
	m_aasideDef = aasideDef;
	m_loc = aaloc;

	AASpecialCombatCircumstance aasccForAtt;
	AASpecialCombatCircumstance aasccForDef;

	//Find out if we have to worry about subs or AA guns when we build the force
	//objects. They need to know because it affects the possible states that a force
	//can pass through.
	if (m_loc == aaLOC_LAND){
		aasccForDef = aaSPECIAL_NONE;
		if (m_aasideDef.bAAGun == aaTRUE)
			aasccForAtt = aaSPECIAL_AA;
		else
			aasccForAtt = aaSPECIAL_NONE;
	}
	else {
		if (m_aasideAtt.aaucCounts.nGetNumUnitsOfType(aaSUB) > 0)
			aasccForDef = aaSPECIAL_SUB;
		else
			aasccForDef = aaSPECIAL_NONE;

		if (m_aasideDef.aaucCounts.nGetNumUnitsOfType(aaSUB) > 0)
			aasccForAtt = aaSPECIAL_SUB;
		else
			aasccForAtt = aaSPECIAL_NONE;
	}
		
	//Create the opposing forces.
	m_forceAtt = AAForce(m_aasideAtt.aaucCounts, m_aasideAtt.aalosspPrefs, m_loc, aaPOS_ATT, m_aasideAtt.aatechTechs, aasccForAtt);
	m_forceDef = AAForce(m_aasideDef.aaucCounts, m_aasideDef.aalosspPrefs, m_loc, aaPOS_DEF, m_aasideDef.aatechTechs, aasccForDef);

	//Initialize the Transition Matrix.
	int nNumAttStates = m_forceAtt.nGetTLSize();
	int nNumDefStates = m_forceDef.nGetTLSize();
	
	int i;
	int j;
	
	//First we need to build its structure.
	m_pTMRow = new double[nNumAttStates * nNumDefStates];
	ASSERT(m_pTMRow != NULL);
	m_ppTM = new double*[nNumAttStates];
	ASSERT(m_ppTM != NULL);
	for (i = 0; i < nNumAttStates; i++){
		m_ppTM[i] = m_pTMRow + i * nNumDefStates;
		for (j = 0; j < nNumDefStates; j++)
			m_ppTM[i][j] = 0.0;
	}

	//Now we can initialize it. Start the opening state with unity.
	m_ppTM[nNumAttStates - 1][nNumDefStates - 1] = 1.0;


	int nAttState;
	int nDefState;
	int nNumAttHits;
	int nNumDefHits;
	AAHitProbDist aahpdAtt;
	AAHitProbDist aahpdDef;
	double dZeroHitsDist;
	double dDist;
	double* pdaAttPs;
	double* pdaDefPs;
	int nASTI;
	int nDSTI;


	//There is further initialization to do if we are on land. We have to worry about
	//anti-air and battleships.
	if (m_loc == aaLOC_LAND){
	
		//Count the attacking planes.
		int nPlanes = m_aasideAtt.aaucCounts.nGetNumUnitsOfType(aaFIGHTER) + m_aasideAtt.aaucCounts.nGetNumUnitsOfType(aaBOMBER);
		
		//We'll want to keep track of all the "initial" states the attacking force
		//might start out in. These are the states where various numbers of AA
		//shots have hit. We also need to track the odds of starting in those
		//states.
		int* naAttStates;
		double* daDists;

		int nNumInitialStates;
	
		//Transit for AA shots, if applicable.
		if (aasccForAtt == aaSPECIAL_AA && nPlanes > 0){
			
			//If there are planes to hit and there is an AA gun, find out the
			//initial states and the probability of starting in each.
			nNumInitialStates = nPlanes + 1;
			naAttStates = new int[nNumInitialStates];
			ASSERT(naAttStates != NULL);
			daDists = new double[nNumInitialStates];
			ASSERT(daDists != NULL);
		
			//Build the AA gun's hit probability distribution.
			double dArr[2];
			dArr[0] = 5.0/6.0;
			dArr[1] = 1.0/6.0;
			AAHitProbDist aahpdSingleAA(dArr, 0, 1);
			AAHitProbDist aahpdAA;
			for (i = 0; i < nPlanes; i++)
				aahpdAA = aahpdAA + aahpdSingleAA;

			ASSERT(nPlanes == aahpdAA.nGetHighHitCount());
			
			double* daPAA = aahpdAA.pdGetProbs();
			
			//Find the states by running down the force's special links. An AA
			//hit is a "special" hit in a land battle because it means air
			//units can die "out of order."
			int nASToChange = nNumAttStates - 1;
			for (j = 0; j < nNumInitialStates; j++){
				ASSERT(j < nNumInitialStates);
				m_ppTM[nASToChange][nNumDefStates - 1] = daPAA[j];
				daDists[j] = daPAA[j];
				naAttStates[j] = nASToChange;
				nASToChange = m_forceAtt.nGetNextStateOnSpecial(nASToChange);
			}
		}
		else{
			
			//There is only one possible starting state.
			nNumInitialStates = 1;
			naAttStates = new int[1];
			ASSERT(naAttStates != NULL);
			daDists = new double[1];
			ASSERT(daDists != NULL);

			*naAttStates = nNumAttStates - 1;
			*daDists = 1.0;
		}

		//Perform further initialization for battleships.
		if (m_aasideAtt.aaucCounts.nGetNumUnitsOfType(aaBATTLESHIP) > 0){
		
			//Now, add battleships to every state we can reach by hitting only
			//special units--that is, air units killed by AA fire in this case.

			//Adding in battleship shots is tricky. It's like the process for
			//"exploding" matrix elements we do below except that the case of
			//zero-hits is not expanded according to the geometric series summation.
			//This corresponds to the fact that battleships get to fire only once.
			//Later (below) the element get exploded "properly."
		
			//The defender's HPD is the same regardless of initial attacking
			//state.
			aahpdDef = m_forceDef.aaugGetUGForState(nNumDefStates - 1).aahpdGetLandHPD();
			pdaDefPs = aahpdDef.pdGetProbs();
		
			//For now, we always have zero lower hit count.
			ASSERT(aahpdDef.nGetLowHitCount() == 0);
		
			for (i = 0; i < nNumInitialStates; i++){
				
				//Do a partial (non-zero-hits distribution) explosion of the element.
			
				//We only want to subtract the AA component...
				m_ppTM[naAttStates[i]][nNumDefStates - 1] = m_ppTM[naAttStates[i]][nNumDefStates - 1] - daDists[i];	

				//Get the HPDs for this combined state.
				aahpdAtt = m_forceAtt.aaugGetUGForState(naAttStates[i]).aahpdGetNonSubsHPD();
				pdaAttPs = aahpdAtt.pdGetProbs();

				//For now, we always have zero lower hit count.
				ASSERT(aahpdAtt.nGetLowHitCount() == 0);
				
				//Get the actual probability arrays.
				
				//Now multiply the HPDs. Add the results back into the main transition
				//matrix.
				
				nDSTI = nNumDefStates - 1;
				
				for (nNumAttHits = 0; nNumAttHits <= aahpdAtt.nGetHighHitCount(); nNumAttHits++){
					
					nASTI = naAttStates[i];
					
					for (nNumDefHits = 0; nNumDefHits <= aahpdDef.nGetHighHitCount(); nNumDefHits++){
					
						ASSERT(nASTI < nNumAttStates);
						ASSERT(nASTI >= 0);
						ASSERT(nDSTI < nNumDefStates);
						ASSERT(nDSTI >= 0);
					
						//Init the proper TM element
						m_ppTM[nASTI][nDSTI] += pdaAttPs[nNumAttHits] * pdaDefPs[nNumDefHits] * daDists[i];
						
						//Find the next attacker index.
						nASTI = m_forceAtt.nGetNextStateOnNormal(nASTI);
					}
					
					//Find the next defender index.
					nDSTI = m_forceDef.nGetNextStateOnNormal(nDSTI);
				}
			}
		} 
	    
	    //Destroy our list of initial states and probabilities.
	    delete[] naAttStates;
	    delete[] daDists;
	}
    
	
	if (m_loc == aaLOC_LAND){
	
		//We have a land battle so we can just explode our states without
		//worrying about any "special" hits. Those were AA hits and have
		//been taken care of above.
	
		//Iterate over the states.
		for (nAttState = nNumAttStates - 1; nAttState > 0; nAttState--){
		
			//Get the attacker's HPD.
			aahpdAtt = m_forceAtt.aaugGetUGForState(nAttState).aahpdGetLandHPD();
			pdaAttPs = aahpdAtt.pdGetProbs();
			
			//For now, we always have zero lower hit count.
			ASSERT(aahpdAtt.nGetLowHitCount() == 0);
		
			for (nDefState = nNumDefStates - 1; nDefState > 0; nDefState--){

				//Get the defender's HPD.
				aahpdDef = m_forceDef.aaugGetUGForState(nDefState).aahpdGetLandHPD();
				pdaDefPs = aahpdDef.pdGetProbs();
			
				//For now, we always have zero lower hit count.
				ASSERT(aahpdDef.nGetLowHitCount() == 0);
			
				//See what the zero hits distribution is, if any. This factor arises
				//from the case where no hits ocur. In an actual battle run, we would
				//just roll again. When calculating probabilities, though, we can't
				//do that. Instead, consider that every possible outcome (if we did
				//"roll again") is just as likely to happen as before. The same thing
				//is true if we again get zero hits. The below factor comes from the
				//result of the sum of a geometric series when the ratio between terms
				//is less than one, which is the case we have here.
				if (aahpdAtt.nGetLowHitCount() == 0 && aahpdDef.nGetLowHitCount() == 0)
					dZeroHitsDist = 1 / (1 - (pdaAttPs[0] * pdaDefPs[0]));
				else
					dZeroHitsDist = 1.0;

				//Get the combined state distribution factor and multiply it
				//and the zero hits distribution factor to get the total dist
				//for the combined state.
				dDist = dZeroHitsDist * m_ppTM[nAttState][nDefState];

				//Now multiply the HPDs. Add the weighted results back into the main transition
				//matrix.
				nDSTI = nDefState;
				for (nNumAttHits = 0; nNumAttHits <= aahpdAtt.nGetHighHitCount(); nNumAttHits++){
					nASTI = nAttState;
					for (nNumDefHits = 0; nNumDefHits <= aahpdDef.nGetHighHitCount(); nNumDefHits++){
					
						//nASTI and nDSTI contain the combined state to add to given
						//the number of hits from each side.

						//Add to the proper TM element
						ASSERT(nASTI <= nAttState);
						ASSERT(nASTI >= 0);
						ASSERT(nDSTI <= nDefState);
						ASSERT(nDSTI >= 0);
						m_ppTM[nASTI][nDSTI] += pdaAttPs[nNumAttHits] * pdaDefPs[nNumDefHits] * dDist;
					
						nASTI = m_forceAtt.nGetNextStateOnNormal(nASTI);
					}//nNumDefHits

					nDSTI = m_forceDef.nGetNextStateOnNormal(nDSTI);
				}//nNumAttHits

				//Now set the pivot element to zero.
				m_ppTM[nAttState][nDefState] = 0.0;
			}//nDefState
		}//nAttState
	}
	else{
		
		//We have a sea battle. Thus, we need to consider sub hits and
		//their "special" nature. Some units might be hit out of order
		//of preference since subs can't hit planes. There is also the
		//attacking subs' first shot advantage to be considered.
	
		AAHitProbDist aahpdAttSub;
		AAHitProbDist aahpdAttNonSub;
		double* pdaAttSubPs;
		double* pdaAttNonSubPs;
		
		AAHitProbDist aahpdDefSub;
		AAHitProbDist aahpdDefNonSub;
		double* pdaDefSubPs;
		double* pdaDefNonSubPs;

		int nDefStateAfterAttSubHits;
		int nAttStateAfterDefSubHits;
		int nNumAttSubHits;					
		int nNumAttNonSubHits;					
		int nNumDefSubHits;					
		int nNumDefNonSubHits;
		
		double dPNoChange;					

		//Iterate over the states.
		for (nAttState = nNumAttStates - 1; nAttState > 0; nAttState--){
		
			//Get the attacker's HPDs. We'll track sub hits separately from 
			//others
			aahpdAttSub = m_forceAtt.aaugGetUGForState(nAttState).aahpdGetSubsHPD();
			aahpdAttNonSub = m_forceAtt.aaugGetUGForState(nAttState).aahpdGetNonSubsHPD();
			pdaAttSubPs = aahpdAttSub.pdGetProbs();
			pdaAttNonSubPs = aahpdAttNonSub.pdGetProbs();
			
			//For now, we always have zero lower hit count.
			ASSERT(aahpdAttSub.nGetLowHitCount() == 0);
			ASSERT(aahpdAttNonSub.nGetLowHitCount() == 0);
		
			for (nDefState = nNumDefStates - 1; nDefState > 0; nDefState--){
				
/*********************/
				dPNoChange = 0.0;
				
				nDefStateAfterAttSubHits = nDefState;

				//Now, let's loop for the number of possible attacking sub hits given
				//this attacker's state.
				for (nNumAttSubHits = 0; nNumAttSubHits <= aahpdAttSub.nGetHighHitCount(); nNumAttSubHits++){ 
					
					//Given this number of sub hits, get the defender's HPDs
					aahpdDefSub = m_forceDef.aaugGetUGForState(nDefStateAfterAttSubHits).aahpdGetSubsHPD();
					aahpdDefNonSub = m_forceDef.aaugGetUGForState(nDefStateAfterAttSubHits).aahpdGetNonSubsHPD();
					pdaDefSubPs = aahpdDefSub.pdGetProbs();
					pdaDefNonSubPs = aahpdDefNonSub.pdGetProbs();
			
					//For now, we always have zero lower hit count.
					ASSERT(aahpdDefSub.nGetLowHitCount() == 0);
					ASSERT(aahpdDefNonSub.nGetLowHitCount() == 0);
			
					nDSTI = nDefStateAfterAttSubHits;
					
					for (nNumAttNonSubHits = 0; nNumAttNonSubHits <= aahpdAttNonSub.nGetHighHitCount(); nNumAttNonSubHits++){ 
						
						nAttStateAfterDefSubHits = nAttState;

						for (nNumDefSubHits = 0; nNumDefSubHits <= aahpdDefSub.nGetHighHitCount(); nNumDefSubHits++){ 
							
							nASTI = nAttStateAfterDefSubHits;

							for (nNumDefNonSubHits = 0; nNumDefNonSubHits <= aahpdDefNonSub.nGetHighHitCount(); nNumDefNonSubHits++){
								
								ASSERT(nASTI <= nAttState);
								ASSERT(nASTI >= 0);
								ASSERT(nDSTI <= nDefState);
								ASSERT(nDSTI >= 0);
								if (nASTI == nAttState && nDSTI == nDefState)
									dPNoChange += pdaAttSubPs[nNumAttSubHits] * pdaAttNonSubPs[nNumAttNonSubHits] * pdaDefSubPs[nNumDefSubHits] * pdaDefNonSubPs[nNumDefNonSubHits];
																	
								nASTI = m_forceAtt.nGetNextStateOnNormal(nASTI);
							
							}//nNumDefNonSubHits

							nAttStateAfterDefSubHits = m_forceAtt.nGetNextStateOnSpecial(nAttStateAfterDefSubHits);

						}//nNumDefSubHits

						nDSTI = m_forceDef.nGetNextStateOnNormal(nDSTI);

					}//nNumAttNonSubHits

					nDefStateAfterAttSubHits = m_forceDef.nGetNextStateOnSpecial(nDefStateAfterAttSubHits);

				}//nNumAttSubHits 

/*********************/

/*				aahpdDefSub = m_forceDef.aaugGetUGForState(nDefState).aahpdGetSubsHPD();
				aahpdDefNonSub = m_forceDef.aaugGetUGForState(nDefState).aahpdGetNonSubsHPD();
				pdaDefSubPs = aahpdDefSub.pdGetProbs();
				pdaDefNonSubPs = aahpdDefNonSub.pdGetProbs();

				//Get the distribution factor.
				if (aahpdAtt.nGetLowHitCount() == 0 && aahpdDef.nGetLowHitCount() == 0)
					dZeroHitsDist = 1 / (1 - (pdaAttSubPs[0] * pdaAttNonSubPs[0] * pdaDefSubPs[0] * pdaDefNonSubPs[0]));
				else
					dZeroHitsDist = 1.0;

				//Get the combined state distribution factor and multiply it
				//and the zero hits distribution factor to get the total dist
				//for the combined state.
				dDist = dZeroHitsDist * m_ppTM[nAttState][nDefState];
*/
				dDist = (1 / (1 - dPNoChange)) * m_ppTM[nAttState][nDefState];

				//We'll need to track the actual defender's state we enter for
				//each possible number of attacking sub hits.
				nDefStateAfterAttSubHits = nDefState;

				//Now, let's loop for the number of possible attacking sub hits given
				//this attacker's state.
				for (nNumAttSubHits = 0; nNumAttSubHits <= aahpdAttSub.nGetHighHitCount(); nNumAttSubHits++){ 
					
					//Given this number of sub hits, get the defender's HPDs
					aahpdDefSub = m_forceDef.aaugGetUGForState(nDefStateAfterAttSubHits).aahpdGetSubsHPD();
					aahpdDefNonSub = m_forceDef.aaugGetUGForState(nDefStateAfterAttSubHits).aahpdGetNonSubsHPD();
					pdaDefSubPs = aahpdDefSub.pdGetProbs();
					pdaDefNonSubPs = aahpdDefNonSub.pdGetProbs();
			
					//For now, we always have zero lower hit count.
					ASSERT(aahpdDefSub.nGetLowHitCount() == 0);
					ASSERT(aahpdDefNonSub.nGetLowHitCount() == 0);
			
					nDSTI = nDefStateAfterAttSubHits;
					
					for (nNumAttNonSubHits = 0; nNumAttNonSubHits <= aahpdAttNonSub.nGetHighHitCount(); nNumAttNonSubHits++){ 
						
						nAttStateAfterDefSubHits = nAttState;

						for (nNumDefSubHits = 0; nNumDefSubHits <= aahpdDefSub.nGetHighHitCount(); nNumDefSubHits++){ 
							
							nASTI = nAttStateAfterDefSubHits;

							for (nNumDefNonSubHits = 0; nNumDefNonSubHits <= aahpdDefNonSub.nGetHighHitCount(); nNumDefNonSubHits++){
								
								ASSERT(nASTI <= nAttState);
								ASSERT(nASTI >= 0);
								ASSERT(nDSTI <= nDefState);
								ASSERT(nDSTI >= 0);
								m_ppTM[nASTI][nDSTI] += pdaAttSubPs[nNumAttSubHits] * pdaAttNonSubPs[nNumAttNonSubHits] * pdaDefSubPs[nNumDefSubHits] * pdaDefNonSubPs[nNumDefNonSubHits] * dDist;
																
								nASTI = m_forceAtt.nGetNextStateOnNormal(nASTI);
							
							}//nNumDefNonSubHits

							nAttStateAfterDefSubHits = m_forceAtt.nGetNextStateOnSpecial(nAttStateAfterDefSubHits);

						}//nNumDefSubHits

						nDSTI = m_forceDef.nGetNextStateOnNormal(nDSTI);

					}//nNumAttNonSubHits

					nDefStateAfterAttSubHits = m_forceDef.nGetNextStateOnSpecial(nDefStateAfterAttSubHits);

				}//nNumAttSubHits 
				
				//Now set the pivot element to zero.
				m_ppTM[nAttState][nDefState] = 0.0;

			}//nDefState
		}//nAttState
	}


	#ifdef _DEBUG
	TRACE("\n\n");
	for (nAttState = nNumAttStates - 1; nAttState >= 0; nAttState--){
		for (nDefState = nNumDefStates - 1; nDefState >= 0; nDefState--){
			TRACE("%f ", m_ppTM[nAttState][nDefState]);
		}
		TRACE("\n");
	}
	#endif

}

AABattle::~AABattle()
{
	//Destroy the TM array.
	delete [] *m_ppTM;	//m_pTMRow, really
	delete [] m_ppTM;
}

int AABattle::nGetNumFinalStates(AACombatPosition aaPos)
{
	if (aaPos == aaPOS_ATT)
		return m_forceAtt.nGetTLSize();
	else
		return m_forceDef.nGetTLSize();
}

AAUnitsCounts AABattle::aaucGetCounts(AACombatPosition aaPos, int nState)
{
	if (aaPos == aaPOS_ATT){
		ASSERT(nState < m_forceAtt.nGetTLSize());
		return m_forceAtt.aaugGetUGForState(nState).aaucGetUnitsCounts();
	}
	else{
		ASSERT(nState < m_forceDef.nGetTLSize());
		return m_forceDef.aaugGetUGForState(nState).aaucGetUnitsCounts();
	}
}

double AABattle::dGetProb(AACombatPosition aaPos, int nState)
{
	if (aaPos == aaPOS_ATT){
		ASSERT(nState < m_forceAtt.nGetTLSize());
		return m_ppTM[nState][0];
	}
	else{
		ASSERT(nState < m_forceDef.nGetTLSize());
		return m_ppTM[0][nState];
	}
}
	
/* ************************************************************************* */
