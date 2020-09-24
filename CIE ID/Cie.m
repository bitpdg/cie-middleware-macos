//
//  Cie.m
//  CIE ID
//
//  Created by Pierluigi De Gregorio on 21/09/2020.
//  Copyright © 2020 IPZS. All rights reserved.
//

#import "Cie.h"

@interface Cie()

@property (strong, nonatomic) NSString* name;
@property (strong, nonatomic) NSString* serialNumber;

@end

@implementation Cie

-(id)init: (NSString*) name serial:(NSString*) serialNumner
{
    self.serialNumber = serialNumner;
    self.name = name;
    
    return self;
}

-(NSString*) getName
{
    return self.name;
}
-(NSString*) getSerialNumner
{
    return self.serialNumber;
}

- (void)encodeWithCoder:(nonnull NSCoder *)coder {
    [coder encodeObject:self.name forKey:@"name"];
    [coder encodeObject:self.serialNumber forKey:@"serialNumber"];
}

- (nullable instancetype)initWithCoder:(nonnull NSCoder *)coder {
    self.name = [coder decodeObjectForKey:@"name"];
    self.serialNumber = [coder decodeObjectForKey:@"serialNumber"];
    return self;
}

-(NSString *)description{
    return [NSString stringWithFormat:@"name = %@; serial = %@", _name, _serialNumber];
}

@end
