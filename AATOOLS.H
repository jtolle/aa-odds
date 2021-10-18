class AAHitProbDist
{
private:
	double *pm_ProbArray;
	int m_nLowHitCount;
	int m_nHighHitCount;
	
public:
	AAHitProbDist();
	AAHitProbDist(const double *, int, int);
	AAHitProbDist(const AAHitProbDist&);
	~AAHitProbDist() {delete[] pm_ProbArray;}
	AAHitProbDist& operator=(const AAHitProbDist&);
	AAHitProbDist operator+(const AAHitProbDist&) const;
	//AAHitProbDist& operator+=(const AAHitProbDist&);
	double* pdGetProbs() const {return pm_ProbArray;}
	int nGetLowHitCount() const {return m_nLowHitCount;}
	int nGetHighHitCount() const {return m_nHighHitCount;}
	
	#ifdef _DEBUG
	void debugDisplay();
	#endif

};

/* ************************************************************************* */

class AATechnologies
{
public:
	AABool bSuperSubs;
	AABool bHeavyBombers;
	AABool bJetFighters;
};

/* ************************************************************************* */

class AAUnitPowers
{
private:
	
	//array is indexed by type and contains HPDs for one of a unit of given
	//type, considering location, position, and techs.
	AAHitProbDist m_aahpdaInitUnitHPDs[aaNUM_UNIT_TYPES];

public:
	AAUnitPowers() {}
	AAUnitPowers(AACombatPosition, AATechnologies);
	AAHitProbDist aahpdGetSingleUnitDist(AAUnitType) const;
};
	

/* ************************************************************************* */

//aauc
class AAUnitsCounts
{
private:
	//array is indexed by type and contains counts
	int m_naUnits[aaNUM_UNIT_TYPES];
	int m_nTotalNumUnits;
public:
	AAUnitsCounts();
	void addUnits(AAUnitType, int);
	void removeUnits(AAUnitType, int);
	int nGetNumUnitsOfType(AAUnitType) const;
	int nGetNumUnitsTotal() const {return m_nTotalNumUnits;}
	AABool operator==(const AAUnitsCounts&);
	CString strDescribe() const;
	#ifdef _DEBUG
	void debugDisplay();
	#endif

};

/* ************************************************************************* */

class AAUnitsGroup 
{
private:
	AAUnitsCounts m_aaucCounts;
	
	AALocation m_aaLoc;
	AATechnologies m_aaTech;
	
	AAHitProbDist m_aahpdSubs;
	AAHitProbDist m_aahpdNonSubs;
	AAHitProbDist m_aahpdLand;

public:
	AAUnitsGroup();
	AAUnitsGroup(AALocation, AATechnologies);
	AAUnitsGroup(const AAUnitsGroup&);
	AAUnitsGroup& operator=(const AAUnitsGroup&);
	void addUnits(AAUnitType, int, const AAUnitPowers&);
	AAUnitsCounts aaucGetUnitsCounts() const {return m_aaucCounts;}
	AAHitProbDist aahpdGetSubsHPD() const {return m_aahpdSubs;}
	AAHitProbDist aahpdGetNonSubsHPD() const {return m_aahpdNonSubs;}
	AAHitProbDist aahpdGetLandHPD() const {return m_aahpdLand;}
	int nGetNumUnitsOfType(AAUnitType aautType) const {return m_aaucCounts.nGetNumUnitsOfType(aautType);}
	int nGetNumUnitsTotal() const {return m_aaucCounts.nGetNumUnitsTotal();}
	
	#ifdef _DEBUG
	void debugDisplay();
	#endif
};

/* ************************************************************************* */

class AALossPreference {
	
	//array is indexed by preference but contains types.
	//The first array entry is the unit type that dies first.
	AAUnitType m_aautPrefs[aaNUM_UNIT_TYPES];
public:
	AALossPreference();
	void setTypePref(AAUnitType, int nPref);
	AAUnitType aautGetTypeFromPref(int nPref) const;
}; 

/* ************************************************************************* */

class AAForce {
private:
	
	//We'll maintain a list (as an array) with elements of this struct.
	struct transListElement {
		AAUnitsGroup aaug;
		int nSpecialLink;
		int nNormalLink;
	};

	//Pointer to what will eventually be start of the array	we use to hold
	//all the possible AAUnitsGroup objects that can represent this force
	//in a battle
	transListElement* pm_TL;
	int m_nListSize;
	
	AAUnitsCounts m_aaucCounts;
	AALossPreference m_aalosspKillOrder;
	AASpecialCombatCircumstance m_specialcc;
	AALocation m_loc;
	AATechnologies m_tech;
	AACombatPosition m_pos;
	AAUnitPowers m_powers;

	AAUnitType aautFindBestUnit(AAUnitsGroup&) const;
	AAUnitType aautFindBestNormalUnit(AAUnitsGroup&) const;
	AABool bUnitIsSpecial(AAUnitType) const;
	AABool bUnitFitsLocation(AAUnitType) const;

public:
	AAForce() {}
	AAForce(const AAUnitsCounts&, const AALossPreference&, AALocation, AACombatPosition, AATechnologies, AASpecialCombatCircumstance);
	~AAForce();
	AAForce& operator=(const AAForce&);
	int nGetTLSize() const {return m_nListSize;}
	AAUnitsGroup aaugGetUGForState(int nElement) const
	{
		ASSERT(nElement <= m_nListSize);
		return pm_TL[nElement].aaug;
	}
	int nGetNextStateOnSpecial(int nElement) const
	{
		ASSERT(nElement <= m_nListSize);
		return pm_TL[nElement].nSpecialLink;
	}
	int nGetNextStateOnNormal(int nElement) const
	{
		ASSERT(nElement <= m_nListSize);
		return pm_TL[nElement].nNormalLink;
	}
};

/* ************************************************************************* */

class AASideInfo {
public:
	AAUnitsCounts aaucCounts;
	AALossPreference aalosspPrefs;
	AATechnologies aatechTechs;
	AABool bAAGun;
};

/* ************************************************************************* */

class AABattle {
private:
	
	AASideInfo m_aasideAtt;
	AASideInfo m_aasideDef;
	AALocation m_loc;
	
	AAForce m_forceAtt;
	AAForce m_forceDef;
	double* m_pTMRow;
	double** m_ppTM;

public:
	AABattle() {}
	AABattle(const AASideInfo&, const AASideInfo&, AALocation);
	~AABattle();
	int nGetNumFinalStates(AACombatPosition);
	AAUnitsCounts aaucGetCounts(AACombatPosition, int);
	double dGetProb(AACombatPosition, int);
};

/* ************************************************************************* */
