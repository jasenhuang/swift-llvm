#import <Foundation/Foundation.h>
#import <CommonCrypto/CommonCryptor.h>
#import "a.h"

void say(NSString* msg)// __attribute__((annotate("my annotation")))
{
    NSLog(@"Hello %@", msg);
}

@implementation TestClass
//#pragma obfuscate begin
+ (void)sayClassHello:(NSString*)msg to:(NSString*)user
{
    NSLog(@"Hello Class %@", msg);
    TestClass* obj = [TestClass new];
    [obj sayInstanceHello:msg to:user];
}
- (void)sayInstanceHello:(NSString*)msg to:(NSString*)user
{
    NSLog(@"Hello Instance %@", msg);
    dispatch_async(dispatch_get_main_queue(), ^{
        NSLog(@"calling block");
        [self sayInstanceHello:msg to:user];
    });
}
+ (NSData *)encrypt:(NSData *)plainData key:(NSData *)key iv:(const void *)iv {
    
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
//#pragma obfuscate end
@end

@interface TestClass (category)
- (void)sayCategoryHello:(NSString*)msg __attribute__((annotate("obfuscate")));
@end

@implementation TestClass (category)
- (void)sayCategoryHello:(NSString*)msg
{
    NSLog(@"Hello Category %@", msg);
    NSLog(@"this is encryp data");
}
@end

