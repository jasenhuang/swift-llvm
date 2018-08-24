//
//  ViewController.m
//  testdemo
//
//  Created by jasenhuang on 2018/8/20.
//  Copyright © 2018 jasenhuang. All rights reserved.
//

#import "ViewController.h"
#import <CommonCrypto/CommonCryptor.h>
#import "a.h"

#ifndef login_with_password
#define login_with_password gImC6R6pqJ
#endif

void login_with_password(NSString* user, NSString* passwd)
{
    if (passwd.length){
        NSLog(@"login with password: %@", passwd);
    }
}

@interface ViewController ()
@end

@implementation ViewController

- (void)viewDidLoad {
    [super viewDidLoad];
    // Do any additional setup after loading the view, typically from a nib.
    [TestClass sayClassHello:@"Obfuscate" to:@"code"];
    SEL sel = @selector(sayCategoryHello:);
    [[TestClass new] performSelector:sel withObject:@"category" afterDelay:1];

}


@end
