#include "AsyncTaskLibrary.h"
#undef UpdateResource
#undef CopyFile

FVersionData UAsyncTaskLibrary::DownloadVersionData(bool& Success)
{
	FVersionData VersionData;

	HINTERNET HOpen = InternetOpen(L"[REDACTED]", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
	HINTERNET HConnect = InternetConnect(HOpen, L"[REDACTED]", INTERNET_DEFAULT_FTP_PORT, L"[REDACTED]", L"[REDACTED]", INTERNET_SERVICE_FTP, INTERNET_FLAG_PASSIVE, 0);
	HINTERNET HFTPOpen = NULL;

	if (HOpen != NULL && HConnect != NULL)
	{
		HFTPOpen = FtpOpenFile(HConnect, L"public_html/main/version.json", GENERIC_READ, FTP_TRANSFER_TYPE_BINARY, NULL);

		if (HFTPOpen != NULL)
		{
			char Buffer[256] = {};
			DWORD BytesRead = 0;

			if (InternetReadFile(HFTPOpen, &Buffer, 256, &BytesRead))
			{
				FString JSON(Buffer);

				if (FJsonObjectConverter::JsonObjectStringToUStruct(JSON, &VersionData, 0, 0)) 
				{
					Success = true;
				}
			}
		}
	}

	InternetCloseHandle(HOpen);
	InternetCloseHandle(HConnect);
	InternetCloseHandle(HFTPOpen);
	return VersionData;
}

FClientPlayerData UAsyncTaskLibrary::DownloadPlayerData(const FName Username, bool& Success)
{
	FClientPlayerData PlayerData;
	Success = false;

	HINTERNET HOpen = InternetOpen(L"[REDACTED]", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
	HINTERNET HConnect = InternetConnect(HOpen, L"[REDACTED]", INTERNET_DEFAULT_FTP_PORT, L"[REDACTED]", L"[REDACTED]", INTERNET_SERVICE_FTP, INTERNET_FLAG_PASSIVE, 0);
	HINTERNET HFTPOpen = NULL;

	if (HOpen != NULL && HConnect != NULL)
	{
		FString UsernameFStr("public_html/user/" + Username.ToString() + "/data.json");
		LPCWSTR lpcwUsername;
		UAsyncTaskLibrary::FStringToLPCWSTR(UsernameFStr, lpcwUsername);

		HINTERNET HFTPOpen = FtpOpenFile(HConnect, lpcwUsername, GENERIC_READ, FTP_TRANSFER_TYPE_BINARY, NULL);

		if (HFTPOpen != NULL)
		{
			char Buffer[4096]; // Large because player can have lots of friends and servers.
			DWORD BytesRead = 0;

			if (InternetReadFile(HFTPOpen, &Buffer, 1024, &BytesRead))
			{
				FString JSON(Buffer);

				if (FJsonObjectConverter::JsonObjectStringToUStruct(JSON, &PlayerData, 0, 0))
				{
					FString ImageDirectory("public_html/user/" + Username.ToString() + "/" + PlayerData.PicturePath);
					PlayerData.ProfilePicture = UAsyncTaskLibrary::DownloadImage(Username, ImageDirectory);

					if (!PlayerData.IconPath.IsEmpty())
					{
						FString IconPath("public_html/user/" + Username.ToString() + "/" + PlayerData.IconPath);
						PlayerData.Icon = UAsyncTaskLibrary::DownloadImage(Username, IconPath);
					}

					Success = true;
				}
			}
		}
	}

	InternetCloseHandle(HOpen);
	InternetCloseHandle(HConnect);
	InternetCloseHandle(HFTPOpen);
	return PlayerData;
}

UTexture2D* UAsyncTaskLibrary::DownloadImage(const FName Username, const FString ImageDirectory)
{
	UTexture2D* LoadedTexture = NULL;

	HINTERNET HOpen = InternetOpen(L"[REDACTED]", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
	HINTERNET HConnect = InternetConnect(HOpen, L"[REDACTED]", INTERNET_DEFAULT_FTP_PORT, L"[REDACTED]", L"[REDACTED]", INTERNET_SERVICE_FTP, INTERNET_FLAG_PASSIVE, 0);

	if (HOpen != NULL && HConnect != NULL)
	{
		FString SaveToDirectory(FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()));
		LPCWSTR lpcwSaveToDirectory;
		FStringToLPCWSTR(SaveToDirectory, lpcwSaveToDirectory);

		FString GetFromDirectory("public_html/user/" + Username.ToString() + "/" + ImageDirectory);
		LPCWSTR lpcwGetFromDirectory;
		FStringToLPCWSTR(GetFromDirectory, lpcwGetFromDirectory);

		if (FtpGetFile(HConnect, lpcwGetFromDirectory, lpcwSaveToDirectory, false, GENERIC_READ, FTP_TRANSFER_TYPE_BINARY, NULL))
		{
			IImageWrapperModule& ImageWrapperModule = FModuleManager::LoadModuleChecked<IImageWrapperModule>(FName("ImageWrapper"));
			TSharedPtr<IImageWrapper> ImageWrapper = ImageWrapperModule.CreateImageWrapper(EImageFormat::PNG);

			TArray<uint8> RawFileData;

			if (FFileHelper::LoadFileToArray(RawFileData, *SaveToDirectory)) {
				if (ImageWrapper.IsValid() && ImageWrapper->SetCompressed(RawFileData.GetData(), RawFileData.Num()))
				{
					const TArray<uint8>* UncompressedBGRA = NULL;

					if (ImageWrapper->GetRaw(ERGBFormat::BGRA, 8, UncompressedBGRA))
					{
						LoadedTexture = UTexture2D::CreateTransient(ImageWrapper->GetWidth(), ImageWrapper->GetHeight(), PF_B8G8R8A8);

						if (!LoadedTexture)
						{
							return NULL;
						}

						void* TextureData = LoadedTexture->PlatformData->Mips[0].BulkData.Lock(LOCK_READ_WRITE);
						FMemory::Memcpy(TextureData, UncompressedBGRA->GetData(), UncompressedBGRA->Num());
						LoadedTexture->PlatformData->Mips[0].BulkData.Unlock();

						LoadedTexture->UpdateResource();
					}
				}
			}
		}
	}

	return LoadedTexture;
}

bool UAsyncTaskLibrary::UploadPlayerData(const FClientPlayerData PlayerData)
{
	HINTERNET HOpen = InternetOpen(L"[REDACTED]", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
	HINTERNET HConnect = InternetConnect(HOpen, L"[REDACTED]", INTERNET_DEFAULT_FTP_PORT, L"[REDACTED]", L"[REDACTED]", INTERNET_SERVICE_FTP, INTERNET_FLAG_PASSIVE, 0);

	FString JSON;
	
	if (FJsonObjectConverter::UStructToJsonObjectString(PlayerData, JSON))
	{
		FString UserDataDirectory("public_html/user/" + PlayerData.Username.ToString() + "/data.json");
		LPCWSTR lpcwUserDataDirectory;
		UAsyncTaskLibrary::FStringToLPCWSTR(UserDataDirectory, lpcwUserDataDirectory);

		if (HOpen != NULL && HConnect != NULL)
		{
			if (FtpOpenFile(HConnect, lpcwUserDataDirectory, GENERIC_WRITE, FTP_TRANSFER_TYPE_BINARY, NULL) == NULL)
			{
				FString SaveToLocalDirectory(FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() + "cache/data.json"));
				FFileHelper::SaveStringToFile(JSON, *SaveToLocalDirectory);
				LPCWSTR lpcwGetFromDirectory;
				UAsyncTaskLibrary::FStringToLPCWSTR(SaveToLocalDirectory, lpcwGetFromDirectory);

				FString NewDirectory("public_html/user/" + PlayerData.Username.ToString());
				LPCWSTR lpcwNewDirectory;
				UAsyncTaskLibrary::FStringToLPCWSTR(NewDirectory, lpcwNewDirectory);

				FtpCreateDirectory(HConnect, lpcwNewDirectory);
				FtpPutFile(HConnect, lpcwGetFromDirectory, lpcwUserDataDirectory, FTP_TRANSFER_TYPE_BINARY, 0);
			}

			HINTERNET HFTPOpen = FtpOpenFile(HConnect, lpcwUserDataDirectory, GENERIC_WRITE, FTP_TRANSFER_TYPE_BINARY, NULL);

			DWORD BytesWritten = 0;
			std::string JSONStr(TCHAR_TO_UTF8(*JSON));
			const char* cJSON = JSONStr.c_str();

			if (InternetWriteFile(HFTPOpen, cJSON, strlen(cJSON), &BytesWritten))
			{
				FString PictureDirectory(TCHAR_TO_UTF8(*PlayerData.PicturePath));
				UAsyncTaskLibrary::UploadImage(PlayerData.Username, PictureDirectory);

				if (PlayerData.Icon != nullptr)
				{
					FString IconDirectory(TCHAR_TO_UTF8(*PlayerData.IconPath));
					UAsyncTaskLibrary::UploadImage(PlayerData.Username, IconDirectory);
				}

				InternetCloseHandle(HOpen);
				InternetCloseHandle(HConnect);
				InternetCloseHandle(HFTPOpen);
				return true;
			}
		}
	}

	InternetCloseHandle(HOpen);
	InternetCloseHandle(HConnect);
	return false;
}

bool UAsyncTaskLibrary::UploadImage(FName Username, const FString ImageDirectory)
{
	HINTERNET HOpen = InternetOpen(L"[REDACTED]", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
	HINTERNET HConnect = InternetConnect(HOpen, L"[REDACTED]", INTERNET_DEFAULT_FTP_PORT, L"[REDACTED]", L"[REDACTED]", INTERNET_SERVICE_FTP, INTERNET_FLAG_PASSIVE, 0);

	if (HOpen != NULL && HConnect != NULL)
	{
		FString LocalDirectory(FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()) + "cache/" + ImageDirectory);
		LPCWSTR lpcwLocalDirectory;
		UAsyncTaskLibrary::FStringToLPCWSTR(LocalDirectory, lpcwLocalDirectory);

		FString SaveToDirectory("public_html/user/" + Username.ToString() + "/" + ImageDirectory);
		LPCWSTR lpcwSaveToDirectory;
		UAsyncTaskLibrary::FStringToLPCWSTR(SaveToDirectory, lpcwSaveToDirectory);
		
		if (FtpPutFile(HConnect, lpcwLocalDirectory, lpcwSaveToDirectory, FTP_TRANSFER_TYPE_BINARY, NULL))
		{
			InternetCloseHandle(HOpen);
			InternetCloseHandle(HConnect);
			return true;
		}
	}

	InternetCloseHandle(HOpen);
	InternetCloseHandle(HConnect);
	return false;
}

FString UAsyncTaskLibrary::GeneratePasswordHash(const FName Username, const FString Password)
{
	FString ToHash(Username.ToString() + Password);
	std::string ToHashStr(TCHAR_TO_UTF8(*ToHash));
	std::string AsHash;

	picosha2::hash256_hex_string(ToHashStr, AsHash);
	FString Hash(AsHash.c_str());

	return Hash;
}

bool UAsyncTaskLibrary::IsLatin(const FString String)
{
	const FRegexPattern LatinPattern(TEXT("^[A-Za-z0-9_]+$"));
	FRegexMatcher Matcher(LatinPattern, String);

	if (!Matcher.FindNext())
	{
		return true;
	}

	return false;
}

TArray<FName> UAsyncTaskLibrary::GetAllUsers(bool& Success)
{
	TArray<FName> UserList;
	Success = false;

	HINTERNET HOpen = InternetOpen(L"[REDACTED]", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
	HINTERNET HConnect = InternetConnect(HOpen, L"[REDACTED]", INTERNET_DEFAULT_FTP_PORT, L"[REDACTED]", L"[REDACTED]", INTERNET_SERVICE_FTP, INTERNET_FLAG_PASSIVE, 0);

	if (HOpen != NULL && HConnect != NULL)
	{
		WIN32_FIND_DATA FindData;
		HINTERNET HFtpFind = FtpFindFirstFile(HConnect, L"public_html/user/", &FindData, INTERNET_FLAG_NEED_FILE, NULL);
		
		FString FirstUsername(FindData.cFileName);
		UserList.Add(FName(*FirstUsername));
		
		WIN32_FIND_DATA NextFindData;

		while (InternetFindNextFile(HFtpFind, &NextFindData))
		{
			FString NextUsername(NextFindData.cFileName);
			UserList.Add(FName(*NextUsername));
		}

		InternetCloseHandle(HFtpFind);
		Success = true;
	}

	InternetCloseHandle(HOpen);
	InternetCloseHandle(HConnect);
	UserList.Remove(FName(TEXT(".")));
	UserList.Remove(FName(TEXT("..")));
	return UserList;
}

void UAsyncTaskLibrary::OpenFileDialog(const FName Username, UTexture2D*& Image)
{
	if (GEngine)
	{
		if (GEngine->GameViewport)
		{
			void* ParentWindowHandle = GEngine->GameViewport->GetWindow()->GetNativeWindow()->GetOSWindowHandle();
			IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();

			if (DesktopPlatform)
			{
				TArray<FString> OutFiles;
				DesktopPlatform->OpenFileDialog(ParentWindowHandle, FString("Upload Image"), FString("C:/Desktop"), FString(""), FString("Image Files | *.jpg;*.png"), 0, OutFiles);
				FString SelectedFileDirectory = OutFiles[0];

				FString SelectedFile = SelectedFileDirectory.RightChop(SelectedFileDirectory.Find(FString("/"), ESearchCase::IgnoreCase, ESearchDir::FromEnd, -1));
				FString UploadDirectory = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()) + "cache/" + SelectedFile;

				const TCHAR* tcSelectedFileDirectory = *SelectedFileDirectory;
				const TCHAR* tcUploadDirectory = *UploadDirectory;

				IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
				PlatformFile.CopyFile(tcUploadDirectory, tcSelectedFileDirectory, EPlatformFileRead::AllowWrite, EPlatformFileWrite::AllowRead);

				UAsyncTaskLibrary::UploadImage(Username, SelectedFile);
			}
		}
	}
}

void UAsyncTaskLibrary::FStringToLPCWSTR(const FString In, LPCWSTR& Out)
{
	std::string InStr(TCHAR_TO_UTF8(*In));
	const char* cIn = InStr.c_str();
	wchar_t wIn[64] = {};
	size_t InSize = strlen(cIn) + 1;
	size_t OutSize;
	mbstowcs_s(&OutSize, wIn, InSize, cIn, InSize - 1);
	Out = wIn;
}