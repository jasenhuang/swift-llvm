//
//  ViewController.h
//  testdemo
//
//  Created by jasenhuang on 2018/8/20.
//  Copyright © 2018 jasenhuang. All rights reserved.
//

#import <UIKit/UIKit.h>

@interface TestClass : NSObject
+ (void)sayClassHello:(NSString*)msg;
- (void)sayInstanceHello:(NSString*)msg;
@end

@interface ViewController : UIViewController
@end

