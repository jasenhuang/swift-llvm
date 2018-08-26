#import <Foundation/Foundation.h>

void say(NSString* msg) ;//__attribute__((annotate("my annotation header")));

@interface TestClass : NSObject
+ (void)sayClassHello:(NSString*)msg to:(NSString*)user __attribute__((annotate("obfuscate")));
- (void)sayInstanceHello:(NSString*)msg to:(NSString*)user __attribute__((annotate("obfuscate")));
- (void)controlFlowFlattening __attribute__((annotate("obfuscate")));
+ (NSData *)encrypt:(NSData *)plainData key:(NSData *)key iv:(const void *)iv __attribute__((annotate("obfuscate")));
@end


