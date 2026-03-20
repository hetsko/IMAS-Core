#include "grpc_backend.h"

#include <algorithm>
#include <numeric>
#include <functional>
#include <cstddef>
#include <grpcpp/grpcpp.h>

// These headers are auto-generated from the *.proto file during compilation
#include "data_transfer.grpc.pb.h"

// #define VERBOSE_GRPC_BACKEND

using namespace data_transfer;

std::string array_path(ArraystructContext *ctx, bool for_dim = false)
{
    std::string path;
    if (for_dim)
    {
        path = ctx->getPath();
    }
    else
    {
        path = ctx->getPath() + "[" + std::to_string(ctx->getIndex()) + "]";
    }
    while (ctx->getParent() != nullptr)
    {
        ctx = ctx->getParent();
        path = ctx->getPath()
                   .append("[")
                   .append(std::to_string(ctx->getIndex()))
                   .append("]/")
                   .append(path);
    }
    return path;
}

std::string array_path(Context *ctx, bool for_dim = false)
{
    auto arr_ctx = dynamic_cast<ArraystructContext *>(ctx);
    if (arr_ctx != nullptr)
    {
        return arr_ctx->getOperationContext()->getDataobjectName() + "/" + array_path(arr_ctx, for_dim);
    }
    auto op_ctx = dynamic_cast<OperationContext *>(ctx);
    if (op_ctx != nullptr)
    {
        return op_ctx->getDataobjectName();
    }
    return "";
}

GRPCBackend::GRPCBackend()
{
}

void GRPCBackend::openPulse(DataEntryContext *ctx, int mode)
{
#ifdef VERBOSE_GRPC_BACKEND
    std::cout << "GRPCBackend::openPulse()" << std::endl;
    std::cout << ctx << std::endl;
#endif

    channel_ = grpc::CreateChannel("127.0.0.1:8383", grpc::InsecureChannelCredentials());
    stub_ = DataTransfer::NewStub(channel_);
}

void GRPCBackend::closePulse(DataEntryContext *ctx, int mode)
{
#ifdef VERBOSE_GRPC_BACKEND
    std::cout << "GRPCBackend::closePulse()" << std::endl;
    std::cout << ctx << std::endl;
#endif

    // TODO: Properly destroy channel connection
    stub_.reset();
    channel_.reset();
}

void GRPCBackend::beginAction(OperationContext *ctx)
{
#ifdef VERBOSE_GRPC_BACKEND
    std::cout << "GRPCBackend::beginAction()" << std::endl;
    std::cout << ctx << std::endl;
#endif
}

void GRPCBackend::endAction(Context *ctx)
{
#ifdef VERBOSE_GRPC_BACKEND
    std::cout << "GRPCBackend::endAction()" << std::endl;
    std::cout << ctx << std::endl;
#endif
}

void GRPCBackend::writeData(Context *ctx,
                            std::string fieldname,
                            std::string timebasename,
                            void *data,
                            int datatype,
                            int dim,
                            int *size)
{
#ifdef VERBOSE_GRPC_BACKEND
    std::cout << "GRPCBackend::writeData()" << std::endl;
    std::cout << ctx << std::endl;
#endif
    throw ALBackendException("writeData: not implemented in gRPC!", LOG);
}

int GRPCBackend::readData(Context *ctx,
                          std::string fieldname,
                          std::string timebasename,
                          void **data,
                          int *datatype,
                          int *dim,
                          int *size)
{
#ifdef VERBOSE_GRPC_BACKEND
    std::cout << "GRPCBackend::readData()" << std::endl;
    std::cout << ctx << std::endl;
#endif

    int return_value = 0;

    switch (*datatype)
    {
    case CHAR_DATA:
        return_value = this->readDataTyped(ctx, fieldname, timebasename, (char **)data, dim, size);
        break;
    case INTEGER_DATA:
        return_value = this->readDataTyped(ctx, fieldname, timebasename, (int **)data, dim, size);
        break;
    case DOUBLE_DATA:
        return_value = this->readDataTyped(ctx, fieldname, timebasename, (double **)data, dim, size);
        break;
    default:
        throw ALBackendException("Unsupported datatype for gRPC backend: " + std::to_string(*datatype), LOG);
    }

    return return_value;
}

int GRPCBackend::readDataTyped(Context *ctx,
                               std::string fieldname,
                               std::string timebasename,
                               char **data,
                               int *dim,
                               int *size)
{
    DataRequestChar request;
    DataResponseChar response;
    grpc::ClientContext grpc_context;
    grpc::Status status;

    request.set_path(array_path(ctx) + "/" + fieldname);
    request.set_timebasepath(timebasename == "" ? timebasename : array_path(ctx) + "/" + timebasename);
    status = this->stub_->GetChar(&grpc_context, request, &response);

    if (!status.ok())
    {
        throw ALBackendException("gRPC failed with error " + std::to_string(status.error_code()) + ": " + status.error_message(), LOG);
    }

    // TODO: This should prevent potential overflow of "size"
    // TODO: Not sure if "size" array is pre-allocated and on what size
    *dim = std::min<std::size_t>(MAXDIM, response.dimsizes_size());
    std::copy_n(response.dimsizes().begin(), *dim, size);

    // TODO: Probably quite questionable and maybe insecure memory allocation?
    // TODO: Size of int should be handled more carefully (32 or 64 bit?), but the rest of IMAS-Core just assumes int == int32_t, so maybe it's fine for now
    const int total_size = response.data_size();
    if (total_size != std::accumulate(size, size + *dim, 1, std::multiplies<>{}))
    {
        throw ALBackendException("Received data size does not match the expected size calculated from dimensions", LOG);
    }
    *data = static_cast<char *>(malloc(total_size * sizeof(char)));
    // Since protobuf does not support char type, we need to convert from uint32_t to char (instead of directly copying the data)
    std::transform(response.data().cbegin(), response.data().cend(), *data,
                   [](std::uint32_t value) -> char
                   {
                       return static_cast<char>(value & 0xFFu);
                   });

    return 1;
}

int GRPCBackend::readDataTyped(Context *ctx,
                               std::string fieldname,
                               std::string timebasename,
                               int **data,
                               int *dim,
                               int *size)
{
    DataRequestInteger request;
    DataResponseInteger response;
    grpc::ClientContext grpc_context;
    grpc::Status status;

    request.set_path(array_path(ctx) + "/" + fieldname);
    request.set_timebasepath(timebasename == "" ? timebasename : array_path(ctx) + "/" + timebasename);
    status = this->stub_->GetInteger(&grpc_context, request, &response);

    if (!status.ok())
    {
        throw ALBackendException("gRPC failed with error " + std::to_string(status.error_code()) + ": " + status.error_message(), LOG);
    }

    // TODO: This should prevent potential overflow of "size"
    // TODO: Not sure if "size" array is pre-allocated and on what size
    *dim = std::min<std::size_t>(MAXDIM, response.dimsizes_size());
    std::copy_n(response.dimsizes().begin(), *dim, size);

    // TODO: Probably quite questionable and maybe insecure memory allocation?
    // TODO: Size of int should be handled more carefully (32 or 64 bit?), but the rest of IMAS-Core just assumes int == int32_t, so maybe it's fine for now
    const int total_size = response.data_size();
    if (total_size != std::accumulate(size, size + *dim, 1, std::multiplies<>{}))
    {
        throw ALBackendException("Received data size does not match the expected size calculated from dimensions", LOG);
    }
    *data = static_cast<int *>(malloc(total_size * sizeof(int)));
    std::copy_n(response.data().begin(), total_size, *data);

    return 1;
}

int GRPCBackend::readDataTyped(Context *ctx,
                               std::string fieldname,
                               std::string timebasename,
                               double **data,
                               int *dim,
                               int *size)
{
    DataRequestDouble request;
    DataResponseDouble response;
    grpc::ClientContext grpc_context;
    grpc::Status status;

    request.set_path(array_path(ctx) + "/" + fieldname);
    request.set_timebasepath(timebasename == "" ? timebasename : array_path(ctx) + "/" + timebasename);
    status = stub_->GetDouble(&grpc_context, request, &response);

    if (!status.ok())
    {
        throw ALBackendException("gRPC failed with error " + std::to_string(status.error_code()) + ": " + status.error_message(), LOG);
    }

    // TODO: This should prevent potential overflow of "size"
    // TODO: Not sure if "size" array is pre-allocated and on what size
    *dim = std::min<std::size_t>(MAXDIM, response.dimsizes_size());
    std::copy_n(response.dimsizes().begin(), *dim, size);

    // TODO: Probably quite questionable and maybe insecure memory allocation?
    // TODO: Size of int should be handled more carefully (32 or 64 bit?), but the rest of IMAS-Core just assumes int == int32_t, so maybe it's fine for now
    const int total_size = response.data_size();
    if (total_size != std::accumulate(size, size + *dim, 1, std::multiplies<>{}))
    {
        throw ALBackendException("Received data size does not match the expected size calculated from dimensions", LOG);
    }
    *data = static_cast<double *>(malloc(total_size * sizeof(double)));
    std::copy_n(response.data().begin(), total_size, *data);

    return 1;
}

void GRPCBackend::deleteData(OperationContext *ctx, std::string path)
{
#ifdef VERBOSE_GRPC_BACKEND
    std::cout << "GRPCBackend::deleteData()" << std::endl;
    std::cout << ctx << std::endl;
#endif

    throw ALBackendException("deleteData: not implemented in gRPC!", LOG);
}

void GRPCBackend::beginArraystructAction(ArraystructContext *ctx, int *size)
{
#ifdef VERBOSE_GRPC_BACKEND
    std::cout << "GRPCBackend::beginArraystructAction()" << std::endl;
    std::cout << ctx << std::endl;
#endif

    ArraystructSizeRequest request;
    ArraystructSizeResponse response;
    grpc::ClientContext grpc_context;
    grpc::Status status;

    auto op_ctx = ctx->getOperationContext();
    auto entry_ctx = op_ctx->getDataEntryContext();

    request.set_path(op_ctx->getDataobjectName() + "/" + array_path(ctx, true));
    status = stub_->GetArraystructSize(&grpc_context, request, &response);

    if (!status.ok())
    {
        throw ALBackendException("gRPC failed with error " + std::to_string(status.error_code()) + ": " + status.error_message(), LOG);
    }

    *size = response.size();
}

std::pair<int, int> GRPCBackend::getVersion(DataEntryContext *ctx)
{
#ifdef VERBOSE_GRPC_BACKEND
    std::cout << "GRPCBackend::getVersion()" << std::endl;
// std::cout << ctx << std::endl;  // SEGFAULT
#endif

    return {GRPC_BACKEND_VERSION_MAJOR, GRPC_BACKEND_VERSION_MINOR};
}

void GRPCBackend::get_occurrences(Context *ctx, const char *ids_name, int **occurrences_list, int *size)
{
#ifdef VERBOSE_GRPC_BACKEND
    std::cout << "GRPCBackend::get_occurrences()" << std::endl;
    std::cout << ctx << std::endl;
#endif

    // TODO: Implement this properly. For now just return a single occurrence with index 0.
    *size = 1;
    *occurrences_list = (int *)malloc(1 * sizeof(int));
    (*occurrences_list)[0] = 0;
}

void GRPCBackend::list_filled_paths(Context *ctx, const char *dataobjectname, char ***path_list, int *size)
{
#ifdef VERBOSE_GRPC_BACKEND
    std::cout << "GRPCBackend::list_filled_paths()" << std::endl;
    std::cout << ctx << std::endl;
#endif

    throw ALBackendException("list_filled_paths: not implemented in gRPC!", LOG);
}
