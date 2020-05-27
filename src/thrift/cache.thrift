
struct GetRequest {
  1: string key
}

struct GetResponse {
    1: string val
}

service CacheService {
    GetResponse Get(1:i32 logid, 2:GetRequest req)
}