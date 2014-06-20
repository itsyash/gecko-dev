/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim:set ts=2 sw=2 sts=2 et cindent: */
/* This Source Code Form is subject to the terms of the Mozilla Public
	* License, v. 2.0. If a copy of the MPL was not distributed with this
	* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef nsSapiService_h
#define nsSapiService_h

#include "mozilla/Mutex.h"
#include "nsAutoPtr.h"
#include "nsTArray.h"
#include "nsIThread.h"
#include "nsISpeechService.h"
#include "nsRefPtrHashtable.h"
#include "mozilla/StaticPtr.h"
#include "mozilla/Monitor.h"

//windows headers

#pragma once

#include <stdio.h>
#include <tchar.h>
#include <atlbase.h>
#include <atlstr.h>
#include <Windows.h>

// SAPI constants
#include <sapi.h>
#include <sphelper.h>
#include <iostream>
#include <vector>

namespace mozilla {
	namespace dom {

		class SapiVoice;
		class SapiCallbackRunnable;

		class nsSapiService : public nsISpeechService
		{
			friend class SapiCallbackRunnable;
		public:
			NS_DECL_THREADSAFE_ISUPPORTS
				NS_DECL_NSISPEECHSERVICE

				nsSapiService();

			virtual ~nsSapiService();

			static nsSapiService* GetInstance();

			static already_AddRefed<nsSapiService> GetInstanceForService();

			static void Shutdown();

		private:

			void Init();

			void RegisterVoices();

			bool mInitialized;

			std::vector<SapiVoice*> mVoices;

			ISpVoice* mVoice;

			Atomic<nsISpeechTask*> mCurrentTask;
			
			
			static StaticRefPtr<nsSapiService> sSingleton;
		};

	} // namespace dom
} // namespace mozilla

#endif
