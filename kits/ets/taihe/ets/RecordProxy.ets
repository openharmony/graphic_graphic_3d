/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

export class ShadowRecord<V> extends Record<string, V> {
    static createProxy<V>(handler: RecordProxyHandler<V>): Record<string, V> {
        return proxy.Proxy.create(new ShadowRecord<V>(), handler) as Record<string, V>;
    }
}

class ShadowRecordIterator<V, R> implements IterableIterator<R> {
    private proxyHandler: RecordProxyHandler<V>;
    private keys: string[];
    private index: int = -1;

    private mapper: (key: string, handler: RecordProxyHandler<V>) => R;

    constructor(proxyHandler: RecordProxyHandler<V>, mapper: (key: string, handler: RecordProxyHandler<V>) => R) {
        this.proxyHandler = proxyHandler;
        this.mapper = mapper;
        this.keys = proxyHandler.getKeys();
    }

    override next(): IteratorResult<R> {
        this.index += 1;
        if (this.index >= this.keys.length) {
            return new IteratorResult<R>();
        }
        const key: string = this.keys[this.index];
        return new IteratorResult<R>(this.mapper(key, this.proxyHandler));
    }

    override $_iterator(): IterableIterator<R> {
        return this;
    }
}

export abstract class RecordProxyHandler<V> extends proxy.DefaultProxyHandler<ShadowRecord<V>> {
    internal recordIterator<R>(fn: (key: String, handle: RecordProxyHandler<V>) => R): IterableIterator<R> {
        return new ShadowRecordIterator<V, R>(this, fn);
    }

    get(target: ShadowRecord<V>, name: string): NullishType {
        if (name == "size") {
            return this.getSize();
        }
        console.error("unexcepted property:", name);
        return null;
    }

    set(target: ShadowRecord<V>, name: string, value: NullishType): boolean {
        console.error("not support setting property.");
        return true;
    }

    invoke(target: ShadowRecord<V>, method: Method, args: FixedArray<NullishType>): NullishType {
        const methodName = method.getName();
        if ("$_get" == methodName || "get" == methodName) {
            const key = args[0] as string;
            return this.getProperty(key);
        } else if ("$_set" == methodName || "set" == methodName) {
            const key = args[0] as string;
            const value = args[1] as V;
            this.setProperty(key, value);
            return target;
        } else if ("keys" == methodName) {
            return this.recordIterator<string>((key: string, handler: RecordProxyHandler<V>) => key);
        } else if ("values" == methodName) {
            return this.recordIterator<V>((key: string, handler: RecordProxyHandler<V>) => handler.getProperty(key)!);
        } else if ("entries" == methodName) {
            return this.recordIterator<[string, V]>((key: string, handler: RecordProxyHandler<V>) => [key,
                handler.getProperty(key)!]);
        }
        return super.invoke(target, method, args);
    }

    abstract getSize(): int;
    abstract getKeys(): string[];
    abstract getProperty(key: string): V | undefined;
    abstract setProperty(key: string, value: V): void;
}