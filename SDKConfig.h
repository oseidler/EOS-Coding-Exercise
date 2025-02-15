// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

struct SDKConfig
{
	/** The product id for the running application, found on the dev portal */
	static constexpr char ProductId[] = "304afb1f260a40f8b62824653a1261cf";

	/** The sandbox id for the running application, found on the dev portal */
	static constexpr char SandboxId[] = "8b7355eee97f494ea9f014bfceeb9416";

	/** The deployment id for the running application, found on the dev portal */
	static constexpr char DeploymentId[] = "3cfbcd6249c240719b3676b0ba5dc63b";

	/** Client id of the service permissions entry, found on the dev portal */
	static constexpr char ClientCredentialsId[] = "xyza7891JcianMhD5swapyNVsQa8Bt2K";

	/** Client secret for accessing the set of permissions, found on the dev portal */
	static constexpr char ClientCredentialsSecret[] = "FpNOeSD6snxB0v5NS5SAfVAhM5WnPRKvW14hXqc37lw";

	/** Game name */
	static constexpr char GameName[] = "Owen Seidler Programming Test";

	/** Encryption key */
	static constexpr char EncryptionKey[] = "1111111111111111111111111111111111111111111111111111111111111111";

	/** Credential name in the DevAuthTool */
	static constexpr char CredentialName[] = "Test Credential";

	/** Host name in the DevAuthTool */
	static constexpr char Port[] = "localhost:31415";
};
