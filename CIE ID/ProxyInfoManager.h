//
//  ProxyInfoManager.h
//  cie-pkcs11
//
//  Created by pdg on 19/03/2021.
//  Copyright © 2021 IPZS. All rights reserved.
//

#ifndef ProxyInfoManager_h
#define ProxyInfoManager_h

@interface ProxyInfoManager : NSObject

-(id)init;
-(NSString *)getEncryptedCredentials: (NSString*)credentials;
-(NSString *)getDecryptedCredentials: (NSString*)encryptedCredentials;
@end
#endif /* ProxyInfoManager_h */
