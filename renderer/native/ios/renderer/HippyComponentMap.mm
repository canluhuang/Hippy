/*!
 * iOS SDK
 *
 * Tencent is pleased to support the open source community by making
 * Hippy available.
 *
 * Copyright (C) 2019 THL A29 Limited, a Tencent company.
 * All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#import "HippyComponentMap.h"

using RootNode = hippy::RootNode;

@interface HippyComponentMap () {
    NSMutableDictionary<NSNumber *, NSMutableDictionary<NSNumber *, id<HippyComponent>> *> *_componentsMap;
    NSMutableDictionary<NSNumber *, id<HippyComponent>> *_rootComponentsMap;
    std::unordered_map<int32_t, std::weak_ptr<RootNode>> _rootNodesMap;
}

@end

@implementation HippyComponentMap

- (instancetype)init {
    self = [super init];
    if (self) {
        _componentsMap = [NSMutableDictionary dictionaryWithCapacity:256];
        _rootComponentsMap = [NSMutableDictionary dictionaryWithCapacity:8];
        _rootNodesMap.reserve(8);
    }
    return self;
}

- (void)addRootComponent:(id<HippyComponent>)component
                rootNode:(std::weak_ptr<hippy::RootNode>)rootNode
                  forTag:(NSNumber *)tag {
    NSAssert(component && tag, @"component &&tag must not be null in method %@", NSStringFromSelector(_cmd));
    if (component && tag && ![_componentsMap objectForKey:tag]) {
        NSMutableDictionary *dic = [NSMutableDictionary dictionaryWithCapacity:256];
        [dic setObject:component forKey:tag];
        [_componentsMap setObject:dic forKey:tag];
        [_rootComponentsMap setObject:component forKey:tag];
        _rootNodesMap[[tag intValue]] = rootNode;
    }
}

- (void)removeRootComponentWithTag:(NSNumber *)tag {
    NSAssert(tag, @"tag must not be null in method %@", NSStringFromSelector(_cmd));
    [_componentsMap removeObjectForKey:tag];
    [_rootComponentsMap removeObjectForKey:tag];
    _rootNodesMap.erase([tag intValue]);
}

- (BOOL)containRootComponentWithTag:(NSNumber *)tag {
    NSAssert(tag, @"tag must not be null in method %@", NSStringFromSelector(_cmd));
    id rootComponent = [self rootComponentForTag:tag];
    return nil != rootComponent;
}

- (__kindof id<HippyComponent>)rootComponentForTag:(NSNumber *)tag {
    NSAssert(tag, @"tag must not be null in method %@", NSStringFromSelector(_cmd));
    return [_rootComponentsMap objectForKey:tag];
}

- (std::weak_ptr<hippy::RootNode>)rootNodeForTag:(NSNumber *)tag {
    return _rootNodesMap[[tag intValue]];
}

- (void)addComponent:(__kindof id<HippyComponent>)component forRootTag:(NSNumber *)tag {
    NSAssert(tag, @"component and tag must not be null in method %@", NSStringFromSelector(_cmd));
    NSAssert([self containRootComponentWithTag:tag], @"no root component for tag:%@", tag);
    NSAssert([component hippyTag], @"component's tag must not be null in %@", NSStringFromSelector(_cmd));
    if (component && tag) {
        id map = [_componentsMap objectForKey:tag];
        [map setObject:component forKey:[component hippyTag]];
    }
}

- (void)removeComponent:(__kindof id<HippyComponent>)component forRootTag:(NSNumber *)tag {
    NSAssert(tag, @"component and tag must not be null in method %@", NSStringFromSelector(_cmd));
    NSAssert([component hippyTag], @"component's tag must not be null in %@", NSStringFromSelector(_cmd));
    if (component && tag) {
        id map = [_componentsMap objectForKey:tag];
        [map removeObjectForKey:[component hippyTag]];
    }
}

- (NSMutableDictionary<NSNumber * ,__kindof id<HippyComponent>> *)componentsForRootTag:(NSNumber *)tag {
    NSAssert(tag, @"tag must not be null in method %@", NSStringFromSelector(_cmd));
    if (tag) {
        id map = [_componentsMap objectForKey:tag];
        return map;
    }
    return nil;
}

- (__kindof id<HippyComponent>)componentForTag:(NSNumber *)componentTag
                                     onRootTag:(NSNumber *)tag {
    NSAssert(componentTag && tag, @"componentTag && tag must not be null in method %@", NSStringFromSelector(_cmd));
    if (componentTag && tag) {
        id map = [_componentsMap objectForKey:tag];
        return [map objectForKey:componentTag];
    }
    return nil;
}

@end
