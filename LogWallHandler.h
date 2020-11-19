#ifndef _LOGWALLHANDLER_H
#define _LOGWALLHANDLER_H

#include <vector>

//#include "../../../cpplib/ph_geoacad.h"
#include "../../../cpplib/ph_AcadUtil.h"
#include "../../../cpplib/ph_AcadUiUtil.h"

// Available from AC22 onwards
#if PH_ACADUTIL_APIVER>=2200

#include "logwall81.h"


typedef CLogMaterials::CMatInfo::CLogType CLogType;


bool LogWall_Toggle();
void LogWall_Destroy();
void LogWall_OnDocChange();

class CLogItem
{
public:
	CLogItem();
	CLogItem(short nId, GS::Int32 nBottomType, const CLogType *prLogType);

	~CLogItem();

	void	SetId(short nIndx);
	void	SetBottomType(GS::Int32 bIsFlatBottom);
	void	SetType(const CLogType *prDestLogType);

	short			GetId()			const;
	GS::Int32		GetBottomType() const;
	const CLogType *GetType()		const;

	// Real rise for halves also
	double GetRealRise() const;

private:
	short			m_nId;
	GS::Int32		m_nBottomType;				// ELogTypeNormal/ELogTypeLowestFull etc.
	const CLogType	*m_prType;					// Ptr to CLogTypeHandler's container
};


class CLogItemHandler
{

public:
	CLogItemHandler()
	{
		m_nTotalHeight = 0;
	}
	~CLogItemHandler()
	{

	}

	const std::vector<CLogItem> *GetLogList() const;

	CLogItem *GetLogById(short nLogIndx);
	CLogItem *GetLogByVecIndx(size_t nVecIndx);
	void	AddLog(const CLogItem &rLog);

	//bool	DeleteLog(short nLogIndx);
	void	DeleteLogs();
	bool	DeleteLog(size_t uLogIndx);

	bool	MoveLog(bool bMoveUp, short nLogIndx);

	size_t	GetCount() const;

private:
	// added logs in correct order
	std::vector<CLogItem>	m_vecLogs;
	int						m_nTotalHeight;
};

/*
* CLogTypeHandler
*/
class CLogTypeHandler
{
public:
	struct SWidthType
	{
		GS::Int32	nID;
		double		dValue;

		SWidthType()
		{
			memset(this, 0, sizeof(*this));
		}
	};

public:
	static bool Sort(const SWidthType &a, const SWidthType &b)
	{
		return a.dValue < b.dValue;
	}

public:
	CLogTypeHandler();


	~CLogTypeHandler()
	{

	}
	void			InitLogWidths();
	void			Init();

	const CLogType *GetLogTypeById(GS::Int32 uLogTypeId)	const;
	const CLogType *GetLogTypeByVecIndx(size_t uLogTypeId)	const;
	const CLogType *GetLogTypeByName(const std::string &rLogName)	const;

	bool	UpdateLogTypesByWidth(double dSelectedWidth);

	bool	IsInited();

	size_t		GetCount();
	size_t		GetWidthCount();

	const SWidthType	*GetWidthByIndx(size_t uIndx);
	const SWidthType	*GetWidthByID(GS::Int32 nID);

public:

	// TODO change the logic for initializing logTypes, this is dumb and disgusting
	static CLogType GetLogTypeByNameAndBottomId(const std::string &rLogTypeName, GS::Int32 nType)
	{
		CLogType rLogTypeResult;

		auto &vecMat = grLogMaterials.GetMatVec();

		for (size_t i = 0; i < vecMat.size(); i++)
		{
			auto &rCurrMat = vecMat.at(i);

			if (rCurrMat.m_sName != rLogTypeName) continue;

			/*
			*	ELogTypeNormal,			// <profile> in xml
			*	ELogTypeLowestFull,		// <profilelowest> in xml
			*	ELogTypeUpperHalf,		// Lowest half log in the wall
			*	ELogTypeLowerHalf
			*/

			rCurrMat.GetLogType(rLogTypeResult, nType);
			break;
		}

		return rLogTypeResult;
	}

private:
	bool	m_bIsInited;

	// predefined log types / profiles 
	std::vector<CLogType>		m_vecLogTypes;
	std::vector<CLogType*>		m_vecLogTypesByWidth;
	std::vector<SWidthType>		m_vecWidths; // == thickness in xml
};

/*
* CLogWallHandler
*/
class CLogWallHandler
{

private:
	/*
	* Remember to dispose memory handles; ACAPI_DisposeAttrDefsHdlsExt( rPofileAttrDef ..)
	*/
	void		BuildProfileImage(ProfileVectorImage* image, const CLogType *prLogTypevoid, double dHeightOffset);
	//void		ListProfileDescription(const ProfileVectorImage& profileDescription, const GS::HashTable<PVI::ProfileParameterId, GS::UniString>& parameterNameTable);

	//GSErrCode	Do_ReadProfileWall();

public:
	CLogWallHandler();
	~CLogWallHandler();

	GSErrCode	CreateProfile(API_Attribute &rProfileAttr, API_AttributeDefExt &rProfileAttrDef);
	GSErrCode	UpdateProfile(API_Attribute &rProfileAttr, API_AttributeDefExt &rProfileAttrDef);

	void		BuildProfileAttribute(const std::vector<CLogItem> &vecLogItems, const std::string &rProfileName, API_Attribute &rDestAttr, API_AttributeDefExt &rProfileAttrDef);

	void		ApplyLogProfileToSelectedWalls(const API_Attribute &rAttr);
};


// Data saved with attribute that tells type of each log layer
class CLogWallAttrData
{
public:
	// Clears this and reads data
	// Returns true=data ok, false=no data or bad data
	bool FromAttr(const API_Attr_Head &rAttrHead);
	bool FromWall(const API_WallType &elemWall);

private:
	bool FromElemData(ph::CAcadElemData &rElemData);

public:
	// Two containers for historic reasons, last is the lowest one (order as in UI's list)
	std::vector<std::string>	m_vecLogNames;
	std::vector<GS::Int32>		m_vecProfileTypes;			// Actually CLogMaterials::CMatInfo::LogType + 1 (NEVER ZERO, 1-BASED)
};


class CProfileAttributeHandler
{
public:
	struct SProfileItemDataHead
	{
		GS::Int32		nLogCount;
		GS::Int32		nReserved;
		//char			sProfileName[64];

		SProfileItemDataHead()
		{
			memset(this, 0, sizeof(*this));
		}
	};

public:
	CProfileAttributeHandler();
	~CProfileAttributeHandler();

	const std::vector<API_Attribute> *GetProfileList();

	API_Attribute *GetProfileByVecIndx(size_t nVecIndx);
	//API_Attribute *GetProfileById(short nProfileId);

	//void	AddProfile(const API_Attribute &rProfileAttr);

	bool	DeleteProfile(const std::string &rProfileName);

	GSErrCode	SaveProfileData(API_Attr_Head &rAttrHead, CLogItemHandler &rLogsHandler);
	GSErrCode	DeleteProfileData(API_Attr_Head &rAttrHead);

	// Use CLogWallAttrData to read the data

	size_t	GetCount();
	void	Init();

private:
	std::vector<API_Attribute>	m_vecProfiles;
};

/*
* CAttributeHelpers
*/
class CAttributeHelpers
{

public:
	static GSErrCode GetAttributeByName(const char *pAttrName, API_AttrTypeID eAttrType, API_Attribute &rDestAttribute);
	static GSErrCode GetAttributeByIndx(API_AttributeIndex nAttrIndx, API_AttrTypeID eAttrType, API_Attribute &rDestAttribute);

	static GSErrCode DeleteAttributeByName(const char *pAttrName, API_AttrTypeID eAttrType);
	static GSErrCode DeleteAttributeByIndx(API_AttributeIndex nAttrIndx, API_AttrTypeID eAttrType);

	static const char* GetAttributeName(API_AttrTypeID inType, API_AttributeIndex inIndex);
};


class CCreateLogsDlg : public DG::Palette,
	public DG::ListBoxObserver,
	public DG::PanelObserver,
	public DG::ButtonItemObserver,
	public DG::UserControlObserver,
	public DG::CheckItemObserver,
	public DG::TextEditObserver,
	public ph::IAcadElemNotifyEvents
{
	

private:
	/*
	* GUI items
	*/
	DG::Button				m_butApply, m_butSaveProfile, m_butDiscardChanges;
	DG::IconButton			m_butAddNewProfile, m_butAddNewLog, m_butMoveUp, m_butMoveDown, m_butDeleteLog, m_butDeleteProfile, m_butDuplicateProfile, m_butSaveName, m_butDiscardName;

	DG::LeftText			m_totalHeight;
	DG::TextEdit			m_txtProfileName;

	DG::SingleSelListBox	m_logList, m_profileList;
	UC::UC257				m_logTypes, m_logWidths, m_logBottoms;

	/*
	* Handlers
	*/
	CProfileAttributeHandler	m_rProfileAttributeHandler;
	CLogItemHandler				m_rLogItemHandler;
	CLogTypeHandler				m_rLogTypeHandler;
	CLogWallHandler				m_rLogsHandler;

	/*
	* Class variables
	*/
	std::map<int, std::string>	m_mapBottomTypes;
	GS::UniString				m_rProfileName;

	double					m_dTotalHeight;
	bool					m_bIsSavedState;

public:
	CCreateLogsDlg();
	~CCreateLogsDlg();

	void SetUIStatus();
	void Show(void);

protected:
	virtual void ListBoxSelectionChanged(const DG::ListBoxSelectionEvent& ev);
	virtual void UserControlChanged(const DG::UserControlChangeEvent& ev);

protected:
	virtual void PanelOpened(const DG::PanelOpenEvent& ev);
	virtual void PanelClosed(const DG::PanelCloseEvent & /*ev*/);
	virtual void PanelCloseRequested(const DG::PanelCloseRequestEvent& ev, bool* accept);
	virtual void CheckItemChanged(const DG::CheckItemChangeEvent& ev);
	virtual void ButtonClicked(const DG::ButtonClickEvent& ev);
	virtual void TextEditChanged(const DG::TextEditChangeEvent& ev);

	// Huom! Jos muokaat elementtiä eventhandlerissä, rukkaa prElem->header.index. Muuten mahdolliset muut tarkkailijat saavat huonon indeksin.
// Huom2! prElem EI KOSKAAN ole NULL
public:
	// KT: Kutsutaan valinnan muuttuessa (NULL=ei valintaa)
	virtual void OnElemSelChange(API_Element *prElem, const API_Neig *selElemNeig);

public:
	// Called after pln-file is opened/libraries changed
	// Initializes content of the palette to match current situation
	void OnDocChanged();

private:
	void	PopulateLogTypes();
	void	PopulateLogBottoms();
	void	PopulateLogWidths();
	void	PopulateUIByAttribute(const API_Attribute &rAttr);

	void	InitBottomTypes();
	
	void	ClearLogList();
	void	DrawLogList(short nSelectedLog = 0);

	void	UpdateLogList(short nSelectedLog = 0);
	void	UpdateLogListUI();
	void	UpdateLogWidth(double dLogWidth);

	/*
	* GUI item event handler functions
	*/
	void	HandleApplyEvent();
	void	HandleMoveUpEvent();
	void	HandleMoveDownEvent();
	void	HandleAddLogEvent();
	void	HandleDeleteLogEvent();
	void	HandleSaveNameEvent();
	void	HandleDiscardNameEvent();
	void	HandleAddNewProfileEvent();
	void	HandleDeleteProfileEvent();
	void	HandleDuplicateProfileEvent();
	void	HandleDiscardChangesEvent();
	void	HandleSaveProfileEvent();
	void	HandleProfileListEvent();
	void	HandleLogListEvent();
	void	HandleLogTypesEvent();
	void	HandleLogWidthsEvent();
	void	HandleLogBottomsEvent();

	void	DeleteLogs();
	void	DeleteSelectedLog();
	
	void	UpdateTotalHeight();

	void	AddProfile();
	void	DeleteProfile();
	void	DrawProfileList();
	void	ClearProfileList();
	int		GetProfileDuplicateCount();
	bool	DoesProfileNameAlreadyExist(const char *pProfileName);
	void	GetSelectedProfile(GS::UniString &rDestProfileName);
	void	Save(const char *pName = nullptr);
	void	UpdateProfileName();
	bool	ValidateProfileName();
	void	SetDefaults();	
	void	RevertPendingChanges();


	bool	QueryForPendingChanges(const char *pName = nullptr);
};
#endif	// PH_ACADUTIL_APIVER>=2200
#endif