//
//  MainWindowSidebarViewController.m
//  DeaDBeeF
//
//  Created by Oleksiy Yakovenko on 7/8/20.
//  Copyright © 2020 Oleksiy Yakovenko. All rights reserved.
//

#import "MainWindowSidebarViewController.h"

@interface MainWindowSidebarViewController ()

@property (weak) IBOutlet NSOutlineView *outlineView;

@end

@implementation MainWindowSidebarViewController

- (void)viewDidLoad {
    [super viewDidLoad];

#if ENABLE_MEDIALIB
    self.mediaLibraryOutlineViewController = [[MediaLibraryOutlineViewController alloc] initWithOutlineView:self.outlineView];
#endif
}


@end
