/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
 * The contents of this file are subject to the Netscape Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 */
  
/*
	Outlook Express (Win32) import module
*/

#include "nsCOMPtr.h"
#include "nsIModule.h"
#include "nsIGenericFactory.h"
#include "nsIServiceManager.h"
#include "nsIRegistry.h"
#include "nsIImportService.h"
#include "nsOEImport.h"
#include "nsCRT.h"
#include "OEDebugLog.h"

static NS_DEFINE_CID(kOEImportCID,       	NS_OEIMPORT_CID);
static NS_DEFINE_CID(kImportServiceCID,		NS_IMPORTSERVICE_CID);
static NS_DEFINE_CID(kRegistryCID,			NS_REGISTRY_CID);

////////////////////////////////////////////////////////////////////

nsresult GetImportModulesRegKey( nsIRegistry *reg, nsRegistryKey *pKey)
{
	nsRegistryKey	nScapeKey;

	nsresult rv = reg->GetSubtree( nsIRegistry::Common, "Netscape", &nScapeKey);
	if (NS_FAILED(rv)) {
		rv = reg->AddSubtree( nsIRegistry::Common, "Netscape", &nScapeKey);
	}
	if (NS_FAILED( rv))
		return( rv);

	nsRegistryKey	iKey;
	rv = reg->GetSubtree( nScapeKey, "Import", &iKey);
	if (NS_FAILED( rv)) {
		rv = reg->AddSubtree( nScapeKey, "Import", &iKey);
	}
	
	if (NS_FAILED( rv))
		return( rv);

	rv = reg->GetSubtree( iKey, "Modules", pKey);
	if (NS_FAILED( rv)) {
		rv = reg->AddSubtree( iKey, "Modules", pKey);
	}

	return( rv);
}

NS_METHOD OERegister(nsIComponentManager *aCompMgr,
                     nsIFile *aPath,
                     const char *registryLocation,
                     const char *componentType)
{
	nsresult rv;

	NS_WITH_SERVICE( nsIRegistry, reg, kRegistryCID, &rv);
	if (NS_FAILED(rv)) {
		IMPORT_LOG0( "*** Import OExpress, ERROR GETTING THE Registry\n");
		return rv;
	}

    rv = reg->OpenWellKnownRegistry(nsIRegistry::ApplicationComponentRegistry);
	if (NS_FAILED(rv)) {
		IMPORT_LOG0( "*** Import OExpress, ERROR OPENING THE REGISTRY\n");
		return( rv);
	}

	nsRegistryKey	importKey;

	rv = GetImportModulesRegKey( reg, &importKey);
	if (NS_FAILED( rv)) {
		IMPORT_LOG0( "*** Import OExpress, ERROR getting Netscape/Import registry key\n");
		return( rv);
	}		
			
	nsRegistryKey	key;
	rv = reg->AddSubtree( importKey, "Outlook Express", &key);
	if (NS_FAILED(rv)) return( rv);

	rv = reg->SetString( key, "Supports", kOESupportsString);
	if (NS_FAILED(rv)) return( rv);
	char *myCID = kOEImportCID.ToString();
	rv = reg->SetString( key, "CLSID", myCID);
	delete [] myCID;
	if (NS_FAILED(rv)) return( rv);  	

	return( rv);
}

NS_GENERIC_FACTORY_CONSTRUCTOR(nsOEImport)

static nsModuleComponentInfo components[] = {
    {	"Outlook Express Import Component", 
		NS_OEIMPORT_CID,
		"component://mozilla/import/import-oe", 
		nsOEImportConstructor,
		&OERegister,
		nsnull
	}
};

NS_IMPL_NSGETMODULE("nsOEImport", components)





