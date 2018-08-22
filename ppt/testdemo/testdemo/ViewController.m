//
//  ViewController.m
//  testdemo
//
//  Created by jasenhuang on 2018/8/20.
//  Copyright © 2018 jasenhuang. All rights reserved.
//

#import "ViewController.h"
#import <CommonCrypto/CommonCryptor.h>

#ifndef login_with_password
#define login_with_password gImC6R6pqJ
#endif

void login_with_password(NSString* user, NSString* passwd)
{
    if (passwd.length){
        NSLog(@"login with password: %@", passwd);
    }
}

@implementation TestClass
+ (void)sayClassHello:(NSString*)msg
{
    NSLog(@"Hello Class %@", msg);
    TestClass* obj = [TestClass new];
    [obj sayInstanceHello:msg];
}
- (void)sayInstanceHello:(NSString*)msg
{
    NSLog(@"Hello Instance %@", msg);
    dispatch_async(dispatch_get_main_queue(), ^{
        NSLog(@"calling block");
    });
}
+ (NSData *)AESEncryptData:(NSData *)plainData key:(NSData *)key iv:(const void *)iv {
    
    NSAssert(plainData.length, @"Invalid plainData");
    
    if (!key.length) {
        return plainData;
    }
    
    size_t dataOutLength;
    NSMutableData *cipherData = [NSMutableData dataWithLength:plainData.length + kCCBlockSizeAES128];
    
    CCCryptorStatus result = CCCrypt(kCCEncrypt,
                                     kCCAlgorithmAES128,
                                     kCCOptionPKCS7Padding,
                                     key.bytes,
                                     key.length,
                                     iv,
                                     plainData.bytes,
                                     plainData.length,
                                     cipherData.mutableBytes,
                                     cipherData.length,
                                     &dataOutLength);
    if (result == kCCSuccess) {
        cipherData.length = dataOutLength;
        return cipherData;
    } else {
        return nil;
    }
}
@end

@interface TestClass (category)
- (void)sayCategoryHello:(NSString*)msg;
@end

@implementation TestClass (category)
- (void)sayCategoryHello:(NSString*)msg
{
    NSLog(@"Hello Category %@", msg);
    NSLog(@"this is encryp data");
}
@end

@interface ViewController ()
@end

@implementation ViewController

- (void)viewDidLoad {
    [super viewDidLoad];
    // Do any additional setup after loading the view, typically from a nib.
    [TestClass sayClassHello:@"Obfuscate"];
}


@end
