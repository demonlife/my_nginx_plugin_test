#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>

static char* ngx_http_mytest(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
static ngx_int_t ngx_http_mytest_handler(ngx_http_request_t *r);

static ngx_http_module_t ngx_http_mytest_module_ctx = {
    NULL, NULL,
    NULL, NULL,
    NULL, NULL,
    NULL, NULL
};

static ngx_command_t ngx_http_mytest_commands[] = {
    {
        ngx_string("mytest"),
        NGX_HTTP_MAIN_CONF | NGX_HTTP_SRV_CONF | NGX_HTTP_LOC_CONF |
        NGX_HTTP_LMT_CONF | NGX_CONF_NOARGS,
        ngx_http_mytest,
        NGX_HTTP_LOC_CONF_OFFSET,
        0,
        NULL
    },
    ngx_null_command
};

ngx_module_t ngx_http_mytest_module = {
    NGX_MODULE_V1,
    &ngx_http_mytest_module_ctx,
    ngx_http_mytest_commands,
    NGX_HTTP_MODULE,
    NULL, NULL, NULL, NULL,
    NULL, NULL, NULL,
    NGX_MODULE_V1_PADDING
};

static char* ngx_http_mytest(ngx_conf_t *cf, ngx_command_t *cmd, void *conf) {
    ngx_http_core_loc_conf_t *clcf;

    // 找到mytest配置项所属的配置块，clcf可以是main,srv,loc级别配置项，即
    // 在每个http{}, server{}内都有一个ngx_http_core_loc_conf_t结构体
    clcf = ngx_http_conf_get_module_loc_conf(cf, ngx_http_core_module);

    // http框架在处理用户请求进行到NGX_HTTP_CONTENT_PHASE阶段时，如果请求的主机
    // 域名，URI与mytest配置项所在的配置块相匹配，就调用该方法
    clcf->handler = ngx_http_mytest_handler;

    return NGX_CONF_OK;
}

static ngx_int_t ngx_http_mytest_handler(ngx_http_request_t *r) {
    if (!(r->method & (NGX_HTTP_GET | NGX_HTTP_HEAD))) {
        return NGX_HTTP_NOT_ALLOWED;
    }

    // 获取头部信息 start
    // 获取如下的头部信息: curl -H "Test:test" http://localhost:port/url
    ngx_list_part_t *part = &r->headers_in.headers.part;
    ngx_table_elt_t *header = part->elts;
    ngx_uint_t i = 0;
    for (i = 0; /* void */; ++i) {
        if (i >= part->nelts) {
            if (part->next == NULL) {
                break;
            }
            part = part->next;
            header = part->elts;
            i = 0;
        }
        if (header[i].hash == 0) {
            continue;
        }
        if (0 == ngx_strncasecmp(header[i].key.data,
                                 (u_char*) "Test",
                                 header[i].key.len)) {
            if (0 == ngx_strncmp(header[i].value.data,
                                 "test", header[i].value.len)) {
                // 处理代码
                // 发送自定义HTTP头部 start
                // 丢弃包体
                ngx_http_discard_request_body(r);
                ngx_table_elt_t* h = ngx_list_push(&r->headers_out.headers);
                if (h == NULL) {
                    return NGX_ERROR;
                }
                h->hash = 1;
                h->key.len = sizeof("TestHeader") -1;
                h->key.data = (u_char*)"TestHeader";
                h->value.len = sizeof("TestValue") - 1;
                h->value.data = (u_char*) "TestValue";

                ngx_str_t type = ngx_string("text/plain");
                ngx_str_t response = ngx_string("Return myself Header!");
                r->headers_out.status = NGX_HTTP_OK;
                r->headers_out.content_length_n = response.len;
                r->headers_out.content_type = type;

                ngx_int_t rc = ngx_http_send_header(r);
                // 如果发送的是一个不含有HTTP包体的响应，此时就可以直接return rc;
                if (rc == NGX_ERROR || rc > NGX_OK || r->header_only) {
                    return rc;
                }
                ngx_buf_t *b;
                // ngx_create_temp_buf是一个宏，用于从nginx的内存池中分配ngx_buf_t结构体
                b = ngx_create_temp_buf(r->pool, response.len);
                if (b == NULL) {
                    return NGX_HTTP_INTERNAL_SERVER_ERROR;
                }

                ngx_memcpy(b->pos, response.data, response.len);
                // 需要设置b->last的值， 如果b->last == b->pos， http框架是不会发送一个字节的包体的
                b->last = b->pos + response.len;
                b->last_buf = 1;
                ngx_chain_t out;
                out.buf = b;
                out.next = NULL;
                // 发送包体，并返回
                return ngx_http_output_filter(r, &out);
            }
        }
    }
    // 获取头部信息 end

    ngx_int_t rc = ngx_http_discard_request_body(r);
    if (rc != NGX_OK) {
        return rc;
    }

    ngx_str_t type = ngx_string("text/plain");
    ngx_str_t response = ngx_string("Hello world!");
    r->headers_out.status = NGX_HTTP_OK;
    r->headers_out.content_length_n = response.len;
    r->headers_out.content_type = type;

    rc = ngx_http_send_header(r);
    // 如果发送的是一个不含有HTTP包体的响应，此时就可以直接return rc;
    if (rc == NGX_ERROR || rc > NGX_OK || r->header_only) {
        return rc;
    }

    ngx_buf_t *b;
    // ngx_create_temp_buf是一个宏，用于从nginx的内存池中分配ngx_buf_t结构体
    b = ngx_create_temp_buf(r->pool, response.len);
    if (b == NULL) {
        return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }

    ngx_memcpy(b->pos, response.data, response.len);
    // 需要设置b->last的值， 如果b->last == b->pos， http框架是不会发送一个字节的包体的
    b->last = b->pos + response.len;
    b->last_buf = 1;
    ngx_chain_t out;
    out.buf = b;
    out.next = NULL;

    // 发送包体，并返回
    return ngx_http_output_filter(r, &out);
}
