//
//  CarouselView.m
//  CIE ID
//


#import "CarouselView.h"
#import "CarouselCard.h"

@interface CarouselView(){
    NSInteger index;
    NSArray <Cie *> *cards;
}

@property (weak) IBOutlet NSView *singleCardContainerView;
@property (weak) IBOutlet NSView *multipleCardContainerView;
@property (weak) IBOutlet NSButton *nextButton;
@property (weak) IBOutlet NSButton *backButton;

@property (weak) IBOutlet CarouselCard *leftCard;
@property (weak) IBOutlet CarouselCard *rightCard;
@property (weak) IBOutlet CarouselCard *mainCard;

@end

@implementation CarouselView

- (instancetype)initWithCoder:(NSCoder *)coder{
    self = [super initWithCoder:coder];
    [self setupView];
    return self;
}

- (instancetype)initWithFrame:(NSRect)frameRect{
    self = [super initWithFrame:frameRect];
    [self setupView];
    return self;
}

- (void) setupView {
    NSView *view = [self viewFromNibForClass];
    
    [view setFrame:[self bounds]];
    [view setAutoresizingMask:NSViewMaxXMargin|NSViewMaxYMargin];
    
    [_leftCard setupWithSizeMode:CarouselCardSizeModeSmall];
    [_rightCard setupWithSizeMode:CarouselCardSizeModeSmall];
    [_mainCard setupWithSizeMode:CarouselCardSizeModeRegular];

    [self addSubview:view];
}

// Loads a XIB file into a view and returns this view.
- (NSView *) viewFromNibForClass {
    NSArray *topLevelObjects;
    
    NSBundle *mainBundle = [NSBundle mainBundle];
    
    if ([mainBundle loadNibNamed:@"CarouselView" owner:self topLevelObjects:&topLevelObjects]) {
        for (id item in topLevelObjects) {
            if ([item isKindOfClass:[NSView class]]) {
                return item;
            }
        }
    }
    return nil;
}

#pragma mark - Public methods

- (void) configureWithCards:(NSArray <Cie *> * _Nonnull)cardList {

    NSAssert([cardList count] >= 1, @"Carousel cards must be at least 1");

    __weak __typeof__(self) weakSelf = self;
    
    if ([cardList count] > 1) {
        dispatch_async(dispatch_get_main_queue(), ^{
            __strong __typeof__(weakSelf) strongSelf = weakSelf;
            [strongSelf.singleCardContainerView setHidden:YES];
            [strongSelf.multipleCardContainerView setHidden:NO];
            [strongSelf.backButton setHidden:NO];
            [strongSelf.nextButton setHidden:NO];
            [strongSelf.rightCard setHidden:NO];
            [strongSelf.leftCard setHidden:[cardList count] == 2];
            [strongSelf.backButton setHidden:[cardList count] < 2];
            [strongSelf.nextButton setHidden:[cardList count] == 2];
        });
    }
    else if ([cardList count] == 1) {
        dispatch_async(dispatch_get_main_queue(), ^{
            __strong __typeof__(weakSelf) strongSelf = weakSelf;
            [strongSelf.singleCardContainerView setHidden:NO];
            [strongSelf.multipleCardContainerView setHidden:YES];
            [strongSelf.backButton setHidden:YES];
            [strongSelf.nextButton setHidden:YES];
            [strongSelf.rightCard setHidden:YES];
            [strongSelf.leftCard setHidden:YES];
        });
    }

    index = 0;
    
    cards = cardList;
    
    [self updateCards];

}

- (Cie *) getSelectedCard {
    return [_mainCard getCard];
}

#pragma mark - IBActions

- (IBAction)removeCardPressed:(id)sender {
    if (self.delegate){
        
        if ([cards count] == 1) {
            if ([self.delegate respondsToSelector:@selector(shouldRemoveAllCards)]){
                [self.delegate shouldRemoveAllCards];
            }
        }
        else if ([self.delegate respondsToSelector:@selector(shouldRemoveCard:)]){
            NSArray <Cie *> *newCards = [self.delegate shouldRemoveCard:[_mainCard getCard]];
            [self configureWithCards:newCards];
        }
    }
    
}

- (IBAction)addCardPressed:(id)sender {
    
    if (self.delegate){
        if ([self.delegate respondsToSelector:@selector(shouldAddCard)]){
            [self.delegate shouldAddCard];
        }
    }
}

- (IBAction)removeAll:(id)sender {

    if (self.delegate){
        if ([self.delegate respondsToSelector:@selector(shouldRemoveAllCards)]){
            [self.delegate shouldRemoveAllCards];
        }
    }
}

- (IBAction)backPressed:(id)sender {
    index--;
    
    if (index < 0) {
        index = [cards count] - 1;
    }
    
    [self updateCards];

}

- (IBAction)nextPressed:(id)sender {
    index++;
    
    if (index > ([cards count] - 1)) {
        index = 0;
    }
    
    [self updateCards];
}

#pragma mark - Private methods

- (void) updateCards {
    NSInteger rightIndex = index + 1;
    
    if (rightIndex > ([cards count] - 1)) {
        rightIndex = 0;
    }
    
    NSInteger leftIndex = index - 1;
    
    if (leftIndex < 0) {
        leftIndex = [cards count] - 1;
    }
    
    [self.mainCard configureWithCard:cards[index]];
    [self.leftCard configureWithCard:cards[leftIndex]];
    [self.rightCard configureWithCard:cards[rightIndex]];
    
    __weak __typeof__(self) weakSelf = self;

    dispatch_async(dispatch_get_main_queue(), ^{
        __strong __typeof__(weakSelf) strongSelf = weakSelf;

        if ([strongSelf->cards count] == 2) {
            if (strongSelf->index == 0) {
                [strongSelf.rightCard setHidden:NO];
                [strongSelf.leftCard setHidden:YES];
                [strongSelf.backButton setHidden:NO];
                [strongSelf.nextButton setHidden:YES];
            }
            else {
                [strongSelf.rightCard setHidden:YES];
                [strongSelf.leftCard setHidden:NO];
                [strongSelf.backButton setHidden:YES];
                [strongSelf.nextButton setHidden:NO];
            }
        }
    });
}

@end
