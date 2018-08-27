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
        //[self sayInstanceHello:msg to:user];
    });
    [self controlFlowFlattening];
    [self controlFlowFlattening1];
    [self controlFlowFlattening2];
    [self controlFlowFlattening3];
}

- (void)controlFlowFlattening __attribute__((annotate("obfuscate")))
{
    CFAbsoluteTime begin = CFAbsoluteTimeGetCurrent();
    NSMutableArray * array = [NSMutableArray array];
    int i = 0 ;
    while(i < 100000){
        ++i;
        [array addObject:@(i)];
    }
    NSLog(@"time = %@", @((CFAbsoluteTimeGetCurrent() - begin) * 1000));
}

// obfuscate (1)
- (void)controlFlowFlattening1 __attribute__((annotate("obfuscate")))
{
    CFAbsoluteTime begin = CFAbsoluteTimeGetCurrent();
    NSMutableArray * array = [NSMutableArray array];
    int i = 0 ;
    int swVar1 = 1;
    while(swVar1 != 0){
        switch (swVar1){
            case 1:
                swVar1 = 2;
                break;
            case 2:
            {
                if (i < 100000){
                    swVar1 = 3;
                }else {
                    swVar1 = 0;
                }
                break;
            }
            case 3:
            {
                [array addObject:@(i++)];
                swVar1 = 2;
                break;
            }
        }
    }
    NSLog(@"time 1 = %@", @((CFAbsoluteTimeGetCurrent() - begin) * 1000));
}

// obfuscate (2)
- (void)controlFlowFlattening2 __attribute__((annotate("obfuscate")))
{
    CFAbsoluteTime begin = CFAbsoluteTimeGetCurrent();
    NSMutableArray * array = [NSMutableArray array];
    int i = 0 ;
    int swVar1 = 1;
    while(swVar1 != 0){
        switch (swVar1){
            case 1:
                swVar1 = 2;
                break;
            case 2:
            {
                int swVar2 = 1;
                while(swVar2 != 0){
                    switch(swVar2){
                        case 1:
                        {
                            if (i < 100000){
                                swVar2 = 2;
                            }else {
                                swVar2 = 3;
                            }
                            break;
                        }
                        case 2:
                        {
                            swVar1 = 3;
                            swVar2 = 0;
                            break;
                        }
                        case 3:
                        {
                            swVar1 = 0;
                            swVar2 = 0;
                            break;
                        }
                    }
                }
                break;
            }
            case 3:
            {
                [array addObject:@(i++)];
                swVar1 = 2;
                break;
            }
        }
    }
    NSLog(@"time 2 = %@", @((CFAbsoluteTimeGetCurrent() - begin) * 1000));
}

// obfuscate (3)
- (void)controlFlowFlattening3 __attribute__((annotate("obfuscate")))
{
    CFAbsoluteTime begin = CFAbsoluteTimeGetCurrent();
    NSMutableArray * array = [NSMutableArray array];
    int i = 0 ;
    int swVar1 = 1;
    while(swVar1 != 0){
        switch (swVar1){
            case 1:
                swVar1 = 2;
                break;
            case 2:
            {
                int swVar2 = 1;
                while(swVar2 != 0){
                    switch(swVar2){
                        case 1:
                        {
                            int swVar3 = 1;
                            while(swVar3 != 0){
                                switch(swVar3){
                                    case 1:
                                    {
                                        if (i < 100000){
                                            swVar3 = 2;
                                        }else {
                                            swVar3 = 3;
                                        }
                                        break;
                                    }
                                    case 2:
                                    {
                                        swVar2 = 2;
                                        swVar3 = 0;
                                        break;
                                    }
                                    case 3:
                                    {
                                        swVar2 = 3;
                                        swVar3 = 0;
                                        break;
                                    }
                                }
                            }
                            break;
                        }
                        case 2:
                        {
                            swVar1 = 3;
                            swVar2 = 0;
                            break;
                        }
                        case 3:
                        {
                            swVar1 = 0;
                            swVar2 = 0;
                            break;
                        }
                    }
                }
                break;
            }
            case 3:
            {
                [array addObject:@(i++)];
                swVar1 = 2;
                break;
            }
        }
    }
    NSLog(@"time 3 = %@", @((CFAbsoluteTimeGetCurrent() - begin) * 1000));
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

