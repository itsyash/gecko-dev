/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim:set ts=2 sw=2 sts=2 et cindent: */
/* This Source Code Form is subject to the terms of the Mozilla Public
	* License, v. 2.0. If a copy of the MPL was not distributed with this
	* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "nsISupports.h"
#include "nsSapiService.h"
#include "nsPrintfCString.h"
#include "nsIWeakReferenceUtils.h"
#include "SharedBuffer.h"
#include "nsISimpleEnumerator.h"

#include "mozilla/dom/nsSynthVoiceRegistry.h"
#include "mozilla/dom/nsSpeechTask.h"

#include "nsString.h"
#include "nsIFile.h"
#include "nsThreadUtils.h"
#include "prenv.h"

#include "mozilla/DebugOnly.h"


enum TtsGenderType {
	TTS_GENDER_NONE,
	TTS_GENDER_MALE,
	TTS_GENDER_FEMALE
};

const wchar_t kAttributesKey[] = L"Attributes";
const wchar_t kGenderValue[] = L"Gender";
const wchar_t kLanguageValue[] = L"Language";

namespace mozilla {
	namespace dom {

		StaticRefPtr<nsSapiService> nsSapiService::sSingleton;

		class SapiVoice
		{
		public:
			SapiVoice() {}
			~SapiVoice() {}

			NS_INLINE_DECL_THREADSAFE_REFCOUNTING(SapiVoice)

				WCHAR *mName;
			WCHAR *mLanguage;
			WCHAR *mUri;
			TtsGenderType mGender;
		};

		class SapiCallbackRunnable : public nsRunnable,
			public nsISpeechTaskCallback
		{
		
		public:
			SapiCallbackRunnable(const nsAString& aText, ISpVoice* aVoice,
				float aRate, float aPitch, nsISpeechTask* aTask,
				nsSapiService* aService)
				: mText(aText)
				, mRate(aRate)
				, mPitch(aPitch)
				, mFirstData(true)
				, mTask(aTask)
				, mVoice(aVoice)
				, mService(aService) { }

			~SapiCallbackRunnable() { }

			NS_DECL_ISUPPORTS_INHERITED
				NS_DECL_NSISPEECHTASKCALLBACK

				NS_IMETHOD Run() MOZ_OVERRIDE;

			bool IsCurrentTask() { return mService->mCurrentTask == mTask; }

		private:

			nsString mText;

			float mRate;

			float mPitch;

			bool mFirstData;

			// We use this pointer to compare it with the current service task.
			// If they differ, this runnable should stop.
			nsISpeechTask* mTask;

			// We hold a strong reference to the service, which in turn holds
			// a strong reference to this voice.
			ISpVoice* mVoice;

			// By holding a strong reference to the service we guarantee that it won't be
			// destroyed before this runnable.
			nsRefPtr<nsSapiService> mService;
		};

		NS_IMPL_ISUPPORTS_INHERITED(SapiCallbackRunnable, nsRunnable, nsISpeechTaskCallback)

			NS_IMETHODIMP SapiCallbackRunnable::Run()
		{

				if (!IsCurrentTask()) {
					// If the task has changed, quit.
					return NS_OK;
				}

				nsISpeechTask* task = mTask;

				task->Setup(this, 1, 16000, 2);
				task->DispatchStart();
				HRESULT hr = CoCreateInstance(CLSID_SpVoice, NULL, CLSCTX_ALL, IID_ISpVoice, (void **)&mVoice);
				hr = mVoice->Speak(mText.BeginReading(), 0, NULL);
				float mElapsedTime = 0.01;
				task->DispatchEnd(mElapsedTime,3);
				return NS_OK;
			}

		NS_IMETHODIMP
			SapiCallbackRunnable::OnPause()
		{
				return NS_OK;
			}

		NS_IMETHODIMP
			SapiCallbackRunnable::OnResume()
		{
				return NS_OK;
			}

		NS_IMETHODIMP
			SapiCallbackRunnable::OnCancel()
		{
				mService->mCurrentTask = nullptr;
				return NS_OK;
			}


		NS_INTERFACE_MAP_BEGIN(nsSapiService)
			NS_INTERFACE_MAP_ENTRY(nsISpeechService)
			NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsISpeechService)
			NS_INTERFACE_MAP_END

			NS_IMPL_ADDREF(nsSapiService)
			NS_IMPL_RELEASE(nsSapiService)

			nsSapiService::nsSapiService()
			: mInitialized(false)
			, mCurrentTask(nullptr)
		{
				nsSapiService::Init();
			}

		void nsSapiService::Init()
		{
			MOZ_ASSERT(!mInitialized);

			CComPtr<IEnumSpObjectTokens> voice_tokens;

			if (FAILED(::CoInitialize(NULL)))
				return;


			if (S_OK != SpEnumTokens(SPCAT_VOICES, NULL, NULL, &voice_tokens))
				return;

			unsigned long voice_count, i = 0;
			voice_tokens->GetCount(&voice_count);
			//cout << " count " << voice_count << endl;
			for (unsigned int i = 0; i < voice_count; i++){
				//cout << i << endl;
				SapiVoice *voice = new SapiVoice();
				CComPtr<ISpObjectToken> voice_token;
				if (S_OK != voice_tokens->Next(1, &voice_token, NULL))
					return;

				// do something with the token here; for example, set the voice
				WCHAR *description = nullptr;
				if (S_OK != SpGetDescription(voice_token, &description))
					return;
				voice->mName = description;

				CComPtr<ISpDataKey> attributes;
				if (S_OK != voice_token->OpenKey(kAttributesKey, &attributes))
					return;

				WCHAR *gender_s = nullptr;
				TtsGenderType gender;
				if (S_OK == attributes->GetStringValue(kGenderValue, &gender_s)){
					if (0 == _wcsicmp(gender_s, L"male"))
						gender = TTS_GENDER_MALE;
					else if (0 == _wcsicmp(gender_s, L"female"))
						gender = TTS_GENDER_FEMALE;
				}
				voice->mGender = gender;

				WCHAR *language = nullptr;
				if (S_OK != attributes->GetStringValue(kLanguageValue, &language))
					return;
				voice->mLanguage = language;

				WCHAR *uri = nullptr;
				voice_token->GetId(&uri);
				voice->mUri = uri;

				/*cout << "voice no " << i << endl;
				cout << "description " << wprintf(L"%s\n", description);
				cout << "gender " << wprintf(L"%s\n", gender_s);
				cout << "language " << wprintf(L"%s\n\n", language);*/

				mVoices.push_back(voice);

				voice_token.Release();
				attributes.Release();
			}
			nsSapiService::RegisterVoices();
		}

		struct VoiceTraverserData
		{
			nsSapiService* mService;
			nsSynthVoiceRegistry* mRegistry;
		};

		void nsSapiService::RegisterVoices()
		{
			VoiceTraverserData data = { this, nsSynthVoiceRegistry::GetInstance() };

			int count = mVoices.size();
			std::vector<SapiVoice*>::iterator it;
			for (it = mVoices.begin(); it != mVoices.end(); it++){
				SapiVoice *aVoice = *it;
				DebugOnly<nsresult> rv =
					data.mRegistry->AddVoice(
					data.mService, nsDependentString(static_cast<char16_t*>(aVoice->mUri)), nsDependentString(static_cast<char16_t*>(aVoice->mName)), nsDependentString(static_cast<char16_t*>(aVoice->mLanguage)), true);
				NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "Failed to add voice");
				rv = data.mRegistry->SetDefaultVoice(nsDependentString(static_cast<char16_t*>(aVoice->mUri)), true);
				NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "Failed to add as default voice");
			}

			mInitialized = true;
		}

		nsSapiService::~nsSapiService()
		{
			mVoice->Release();
			mVoice = NULL;
			::CoUninitialize();
		}

		NS_IMETHODIMP nsSapiService::Speak(const nsAString& aText, const nsAString& aUri,
			float aRate, float aPitch, nsISpeechTask* aTask)
		{
			NS_ENSURE_TRUE(mInitialized, NS_ERROR_NOT_AVAILABLE);

			::CoInitialize(NULL);

			mCurrentTask = aTask;

			nsRefPtr<SapiCallbackRunnable> cb = new SapiCallbackRunnable(aText, mVoice, aRate, aPitch, aTask, this);

			NS_DispatchToMainThread(cb);

		}

		NS_IMETHODIMP
			nsSapiService::GetServiceType(SpeechServiceType* aServiceType)
		{
				*aServiceType = nsISpeechService::SERVICETYPE_INDIRECT_AUDIO;
				return NS_OK;
			}

		nsSapiService*
			nsSapiService::GetInstance()
		{
				MOZ_ASSERT(NS_IsMainThread());
				if (XRE_GetProcessType() != GeckoProcessType_Default) {
					MOZ_ASSERT(false, "nsPicoService can only be started on main gecko process");
					return nullptr;
				}

				if (!sSingleton) {
					sSingleton = new nsSapiService();
				}

				return sSingleton;
			}

		already_AddRefed<nsSapiService>
			nsSapiService::GetInstanceForService()
		{
				nsRefPtr<nsSapiService> sapiService = GetInstance();
				return sapiService.forget();
			}

		void
			nsSapiService::Shutdown()
		{
				if (!sSingleton) {
					return;
				}

				sSingleton->mCurrentTask = nullptr;

				sSingleton = nullptr;
			}

	} // namespace dom
} // namespace mozilla
