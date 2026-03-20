#ifndef GRPC_BACKEND_H
#define GRPC_BACKEND_H 1

#include "al_backend.h"
#include <fstream>
#include <complex>
#include <grpcpp/grpcpp.h>

// These headers are auto-generated from the *.proto file during compilation
#include "data_transfer.grpc.pb.h"

static const int GRPC_BACKEND_VERSION_MAJOR = 0;
static const int GRPC_BACKEND_VERSION_MINOR = 0;

#if defined(_WIN32)
#define IMAS_CORE_LIBRARY_API __declspec(dllexport)
#else
#define IMAS_CORE_LIBRARY_API
#endif

#ifdef __cplusplus

class IMAS_CORE_LIBRARY_API GRPCBackend : public Backend
{

private:
    bool verbose_ = false;
    std::shared_ptr<grpc::Channel> channel_;
    std::unique_ptr<data_transfer::DataTransfer::Stub> stub_;

    int readDataTyped(Context *ctx,
                      std::string fieldname,
                      std::string timebasename,
                      char **data,
                      int *dim,
                      int *size);
    int readDataTyped(Context *ctx,
                      std::string fieldname,
                      std::string timebasename,
                      int **data,
                      int *dim,
                      int *size);
    int readDataTyped(Context *ctx,
                      std::string fieldname,
                      std::string timebasename,
                      double **data,
                      int *dim,
                      int *size);

public:
    GRPCBackend();
    virtual ~GRPCBackend() {};

    void openPulse(DataEntryContext *ctx, int mode) override;
    void closePulse(DataEntryContext *ctx, int mode) override;
    void beginAction(OperationContext *ctx) override;
    void endAction(Context *ctx) override;
    void writeData(Context *ctx,
                   std::string fieldname,
                   std::string timebasename,
                   void *data,
                   int datatype,
                   int dim,
                   int *size) override;

    int readData(Context *ctx,
                 std::string fieldname,
                 std::string timebasename,
                 void **data,
                 int *datatype,
                 int *dim,
                 int *size) override;

    void deleteData(OperationContext *ctx, std::string path) override;

    void beginArraystructAction(ArraystructContext *ctx, int *size) override;

    std::pair<int, int> getVersion(DataEntryContext *ctx) override;

    void get_occurrences(Context *ctx, const char *ids_name, int **occurrences_list, int *size) override;
    void list_filled_paths(Context *ctx, const char *dataobjectname, char ***path_list, int *size) override;

    bool supportsTimeDataInterpolation() override
    {
        return false;
    }

    void initDataInterpolationComponent() override
    {
        throw ALBackendException("gRPC backend does not support time range and time slices operations", LOG);
    }

    bool supportsTimeRangeOperation() override
    {
        return false;
    }
};

#endif

#endif // GRPC_BACKEND_H
