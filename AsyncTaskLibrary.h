#pragma once

#include "CoreMinimal.h"
#include "AsyncTask.h"
#include "AsyncWork.h"
#include "EngineMinimal.h"
#include "Engine/Texture2D.h"
#include "EngineMinimal.h"
#include "Json.h"
#include "JsonObjectConverter.h"
#include "GenericPlatform/GenericPlatformFile.h"
#include "NetworkPlatformFile.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "IImageWrapper.h"
#include "IImageWrapperModule.h"
#include "ModuleManager.h"
#include "SlateApplication.h"
#include "Developer/DesktopPlatform/Public/IDesktopPlatform.h"
#include "Developer/DesktopPlatform/Public/DesktopPlatformModule.h"

#include "../Intermediate/ProjectFiles/PicoSHA2.h"

#include "Windows.h"
#include "WinInet.h"
#include <string>

#include "AsyncTaskLibrary.generated.h"

USTRUCT(BlueprintType)
struct FVersionData
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Version;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString DownloadURL;

	FVersionData()
	{
		Version = 0.0F;
		DownloadURL = "NULL";
	}
};

USTRUCT(BlueprintType)
struct FClientPlayerData
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName Username;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString PasswordHash;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString PicturePath;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString IconPath;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Wins;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Losses;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bIsAdmin;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bIsBanned;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FName> Friends;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TMap<FString, bool> Servers;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UTexture2D* ProfilePicture;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UTexture2D* Icon;

	FClientPlayerData()
	{
		Username = "None";
		PasswordHash = "";
		PicturePath = "";
		IconPath = "";
		Wins = 0;
		Losses = 0;
		bIsAdmin = false;
		bIsBanned = false;
		ProfilePicture = nullptr;
		Icon = nullptr;
	}
};

USTRUCT(BlueprintType)
struct FServerPlayerData
{
	GENERATED_BODY();
};

class FMultithreadedTask : public FNonAbandonableTask
{
public:
	UObject* Object;

	FMultithreadedTask(UObject* Object)
	{
		this->Object = Object;
	}

	FORCEINLINE TStatId GetStatId() const
	{
		RETURN_QUICK_DECLARE_CYCLE_STAT(FMultithreadedTask, STATGROUP_ThreadPoolAsyncTasks);
	}

	void DoWork()
	{
		IAsyncTask::Execute_OnAsyncTask(Object);
		Abandon();
	}
};

UCLASS()
class ACCOUNTTEST_API UAsyncTaskLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/* Calls the OnAsyncTask event on another thread. */
	UFUNCTION(BlueprintCallable, meta = (DefaultToSelf = "Object", HidePin = "Object"))
		static void CallTaskAsynchronously(UObject* Object)
	{
		(new FAutoDeleteAsyncTask<FMultithreadedTask>(Object))->StartBackgroundTask();
	}

	/* Downloads information about the version from the FTP Server.	*/
	/* Fails if the server is down, took too long to connect, or the file could not be found. */
	/* NOTE: Should be ran async or the main thread is blocked. */
	UFUNCTION(BlueprintCallable, Category = "Async")
	static FVersionData DownloadVersionData(bool& Success);

	/* Downloads information about a player from the FTP Server. */
	/* Fails if the server is down, took too long to connect, or the file could not be found. */
	/* @param Username - The owner of the information to download. */
	/* NOTE: Should be ran async or the main thread is blocked. */
	UFUNCTION(BlueprintCallable, Category = "Async")
	static FClientPlayerData DownloadPlayerData(const FName Username, bool& Success);

	/* Downloads an image from the FTP Server. */
	/* @param Username - The user folder to browse. */
	/* @param ImageDirectory - The name and lower directory of the image to download. */
	/* Image saved to project/saved/cache */
	/* NOTE: Used internally. */
	static UTexture2D* DownloadImage(const FName Username, const FString ImageDirectory);

	/* Uploads information about a player to the FTP Server. */
	/* Fails if the server is down, took too long to connect, or the file could not be found. */
	/* @param PlayerData - The information to upload. */
	/* NOTE: Should be ran async or the main thread is blocked. */
	UFUNCTION(BlueprintCallable, Category = "Async")
	static bool UploadPlayerData(const FClientPlayerData PlayerData);

	/* Uploads an image to the FTP Server. */
	/* @param Username - The user folder to upload the image to. */
	/* @param ImageDirectory The local image directory, usually project/saved/cache/ */
	/* NOTE: Used internally. */
	static bool UploadImage(FName Username, const FString ImageDirectory);

	/* Generates and salts a hash for the user based on a given password. */
	/* @param Username - The user who's password hash is being generated. */
	/* @param Password - The password to hash and salt. */
	UFUNCTION(BlueprintCallable)
	static FString GeneratePasswordHash(const FName Username, const FString Password);

	/* Checks if a given string contains any non Latin characters. */
	UFUNCTION(BlueprintCallable, BlueprintPure)
	static bool IsLatin(const FString String);

	/* Returns the names of all the folders in the user directory. */
	/* Takes a long time to update. */
	/* NOTE: Should be ran async or the main thread is blocked. */
	UFUNCTION(BlueprintCallable, Category = "Async")
	static TArray<FName> GetAllUsers(bool& Success);

	/* Opens a file dialog to upload an image. */
	UFUNCTION(BlueprintCallable)
	static void OpenFileDialog(const FName Username, UTexture2D*& Image);

	/* Converts an FString to an LPCWSTR. */
	/* NOTE: Used internally. */
	static void FStringToLPCWSTR(const FString In, LPCWSTR& Out);
};