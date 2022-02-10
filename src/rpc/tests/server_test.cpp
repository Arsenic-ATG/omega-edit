/**********************************************************************************************************************
 * Copyright (c) 2022 Concurrent Technologies Corporation.                                                            *
 *                                                                                                                    *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance     *
 * with the License.  You may obtain a copy of the License at                                                         *
 *                                                                                                                    *
 *     http://www.apache.org/licenses/LICENSE-2.0                                                                     *
 *                                                                                                                    *
 * Unless required by applicable law or agreed to in writing, software is distributed under the License is            *
 * distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or                   *
 * implied.  See the License for the specific language governing permissions and limitations under the License.       *
 *                                                                                                                    *
 **********************************************************************************************************************/

#include "../../lib/impl_/macros.h"
#include "omega_edit.grpc.pb.h"
#include <grpcpp/grpcpp.h>
#include <sstream>
#include <thread>

using grpc::Channel;
using grpc::ClientContext;
using grpc::ClientReader;
using grpc::Status;

using omega_edit::Change;
using omega_edit::ChangeKind;
using omega_edit::ChangeRequest;
using omega_edit::ChangeResponse;
using omega_edit::CreateSessionRequest;
using omega_edit::CreateSessionResponse;
using omega_edit::CreateViewportRequest;
using omega_edit::CreateViewportResponse;
using omega_edit::Editor;
using omega_edit::ObjectId;
using omega_edit::SaveSessionRequest;
using omega_edit::SaveSessionResponse;
using omega_edit::VersionResponse;
using omega_edit::ViewportChange;
using omega_edit::ViewportDataRequest;
using omega_edit::ViewportDataResponse;

std::mutex write_mutex;

class OmegaEditServiceClient final {
public:
    explicit OmegaEditServiceClient(const std::shared_ptr<Channel> &channel) : stub_(Editor::NewStub(channel)) {
        assert(channel);
        assert(stub_);
    };

    ~OmegaEditServiceClient() {
        if (viewport_subscription_handler_thread_) { viewport_subscription_handler_thread_->join(); }
    }

    std::string GetOmegaEditVersion() const {
        VersionResponse response;
        google::protobuf::Empty request;
        ClientContext context;

        std::mutex mu;
        std::condition_variable cv;
        bool done = false;
        Status status;
        stub_->async()->GetOmegaVersion(&context, &request, &response, [&mu, &cv, &done, &status](Status s) {
            status = std::move(s);
            std::lock_guard<std::mutex> lock(mu);
            done = true;
            cv.notify_one();
        });

        std::unique_lock<std::mutex> lock(mu);
        while (!done) { cv.wait(lock); }

        if (status.ok()) {
            std::stringstream ss;
            ss << response.major() << "." << response.minor() << "." << response.patch();
            return ss.str();
        } else {
            DBG(CLOG << LOCATION << status.error_code() << ": " << status.error_message() << std::endl;);
            return "RPC failed";
        }
    }

    std::string CreateSession(std::string file_path = std::string()) const {
        CreateSessionRequest request;
        CreateSessionResponse response;
        ClientContext context;

        if (!file_path.empty()) { request.set_file_path(file_path); }
        std::mutex mu;
        std::condition_variable cv;
        bool done = false;
        Status status;
        stub_->async()->CreateSession(&context, &request, &response, [&mu, &cv, &done, &status](Status s) {
            status = std::move(s);
            std::lock_guard<std::mutex> lock(mu);
            done = true;
            cv.notify_one();
        });

        std::unique_lock<std::mutex> lock(mu);
        while (!done) { cv.wait(lock); }

        if (status.ok()) {
            auto session_id = response.session_id().id();
            assert(!session_id.empty());
            return session_id;
        } else {
            DBG(CLOG << LOCATION << status.error_code() << ": " << status.error_message() << std::endl;);
            return "RPC failed";
        }
    }

    int64_t Insert(const std::string &session_id, int64_t offset, const std::string &str) const {
        assert(!session_id.empty());
        ChangeRequest request;
        ChangeResponse response;
        ClientContext context;

        request.mutable_session_id()->set_id(session_id);
        request.set_kind(ChangeKind::CHANGE_INSERT);
        request.set_offset(offset);
        request.set_length(str.length());
        request.set_data(str);

        std::mutex mu;
        std::condition_variable cv;
        bool done = false;
        Status status;
        stub_->async()->SubmitChange(&context, &request, &response, [&mu, &cv, &done, &status](Status s) {
            status = std::move(s);
            std::lock_guard<std::mutex> lock(mu);
            done = true;
            cv.notify_one();
        });

        std::unique_lock<std::mutex> lock(mu);
        while (!done) { cv.wait(lock); }

        if (status.ok()) {
            return response.serial();
        } else {
            DBG(CLOG << LOCATION << status.error_code() << ": " << status.error_message() << std::endl;);
            return 0;
        }
    }

    int64_t Overwrite(const std::string &session_id, int64_t offset, const std::string &str) const {
        assert(!session_id.empty());
        ChangeRequest request;
        ChangeResponse response;
        ClientContext context;

        request.mutable_session_id()->set_id(session_id);
        request.set_kind(ChangeKind::CHANGE_OVERWRITE);
        request.set_offset(offset);
        request.set_length(str.length());
        request.set_data(str);

        std::mutex mu;
        std::condition_variable cv;
        bool done = false;
        Status status;
        stub_->async()->SubmitChange(&context, &request, &response, [&mu, &cv, &done, &status](Status s) {
            status = std::move(s);
            std::lock_guard<std::mutex> lock(mu);
            done = true;
            cv.notify_one();
        });

        std::unique_lock<std::mutex> lock(mu);
        while (!done) { cv.wait(lock); }

        if (status.ok()) {
            return response.serial();
        } else {
            DBG(CLOG << LOCATION << status.error_code() << ": " << status.error_message() << std::endl;);
            return 0;
        }
    }

    int64_t Delete(const std::string &session_id, int64_t offset, int64_t length) const {
        assert(!session_id.empty());
        ChangeRequest request;
        ChangeResponse response;
        ClientContext context;

        request.mutable_session_id()->set_id(session_id);
        request.set_kind(ChangeKind::CHANGE_DELETE);
        request.set_offset(offset);
        request.set_length(length);

        std::mutex mu;
        std::condition_variable cv;
        bool done = false;
        Status status;
        stub_->async()->SubmitChange(&context, &request, &response, [&mu, &cv, &done, &status](Status s) {
            status = std::move(s);
            std::lock_guard<std::mutex> lock(mu);
            done = true;
            cv.notify_one();
        });

        std::unique_lock<std::mutex> lock(mu);
        while (!done) { cv.wait(lock); }

        if (status.ok()) {
            return response.serial();
        } else {
            DBG(CLOG << LOCATION << status.error_code() << ": " << status.error_message() << std::endl;);
            return 0;
        }
    }

    std::string SaveSession(const std::string &session_id, const std::string &file_path) const {
        assert(!session_id.empty());
        assert(!file_path.empty());
        SaveSessionRequest request;
        SaveSessionResponse response;
        ClientContext context;

        request.mutable_session_id()->set_id(session_id);
        request.set_file_path(file_path);

        std::mutex mu;
        std::condition_variable cv;
        bool done = false;
        Status status;
        stub_->async()->SaveSession(&context, &request, &response, [&mu, &cv, &done, &status](Status s) {
            status = std::move(s);
            std::lock_guard<std::mutex> lock(mu);
            done = true;
            cv.notify_one();
        });

        std::unique_lock<std::mutex> lock(mu);
        while (!done) { cv.wait(lock); }

        if (status.ok()) {
            return response.file_path();
        } else {
            DBG(CLOG << LOCATION << status.error_code() << ": " << status.error_message() << std::endl;);
            return "RPC failed";
        }
    }

    std::string DestroySession(const std::string &session_id) const {
        assert(!session_id.empty());
        ObjectId request;
        ObjectId response;
        ClientContext context;

        request.set_id(session_id);

        std::mutex mu;
        std::condition_variable cv;
        bool done = false;
        Status status;
        stub_->async()->DestroySession(&context, &request, &response, [&mu, &cv, &done, &status](Status s) {
            status = std::move(s);
            std::lock_guard<std::mutex> lock(mu);
            done = true;
            cv.notify_one();
        });

        std::unique_lock<std::mutex> lock(mu);
        while (!done) { cv.wait(lock); }

        if (status.ok()) {
            return response.id();
        } else {
            DBG(CLOG << LOCATION << status.error_code() << ": " << status.error_message() << std::endl;);
            return "RPC failed";
        }
    }

    std::string CreateViewport(const std::string &session_id, int64_t offset, int64_t capacity) {
        assert(!session_id.empty());
        CreateViewportRequest request;
        CreateViewportResponse response;
        ClientContext context;

        request.mutable_session_id()->set_id(session_id);
        request.set_offset(offset);
        request.set_capacity(capacity);

        std::mutex mu;
        std::condition_variable cv;
        bool done = false;
        Status status;
        stub_->async()->CreateViewport(&context, &request, &response, [&mu, &cv, &done, &status](Status s) {
            status = std::move(s);
            std::lock_guard<std::mutex> lock(mu);
            done = true;
            cv.notify_one();
        });

        std::unique_lock<std::mutex> lock(mu);
        while (!done) { cv.wait(lock); }

        if (status.ok()) {
            auto viewport_id = response.viewport_id().id();
            assert(!viewport_id.empty());
            return viewport_id;
        } else {
            DBG(CLOG << LOCATION << status.error_code() << ": " << status.error_message() << std::endl;);
            return "RPC failed";
        }
    }

    std::string GetViewportData(const std::string &viewport_id) {
        assert(!viewport_id.empty());
        ViewportDataRequest request;
        ViewportDataResponse response;
        ClientContext context;

        request.mutable_viewport_id()->set_id(viewport_id);

        std::mutex mu;
        std::condition_variable cv;
        bool done = false;
        Status status;
        stub_->async()->GetViewportData(&context, &request, &response, [&mu, &cv, &done, &status](Status s) {
            status = std::move(s);
            std::lock_guard<std::mutex> lock(mu);
            done = true;
            cv.notify_one();
        });

        std::unique_lock<std::mutex> lock(mu);
        while (!done) { cv.wait(lock); }

        if (status.ok()) {
            return response.data();
        } else {
            DBG(CLOG << LOCATION << status.error_code() << ": " << status.error_message() << std::endl;);
            return "RPC failed";
        }
    }

    std::string DestroyViewport(const std::string &viewport_id) const {
        assert(!viewport_id.empty());
        ObjectId request;
        ObjectId response;
        ClientContext context;

        request.set_id(viewport_id);

        std::mutex mu;
        std::condition_variable cv;
        bool done = false;
        Status status;
        stub_->async()->DestroyViewport(&context, &request, &response, [&mu, &cv, &done, &status](Status s) {
            status = std::move(s);
            std::lock_guard<std::mutex> lock(mu);
            done = true;
            cv.notify_one();
        });

        std::unique_lock<std::mutex> lock(mu);
        while (!done) { cv.wait(lock); }

        if (status.ok()) {
            return response.id();
        } else {
            DBG(CLOG << LOCATION << status.error_code() << ": " << status.error_message() << std::endl;);
            return "RPC failed";
        }
    }

    void HandleViewportChanges(std::unique_ptr<ClientContext> context_ptr,
                               std::unique_ptr<ClientReader<ViewportChange>> reader_ptr) {
        assert(reader_ptr);
        (void) context_ptr;// Though we're not using the context pointer, we need to manage its lifecycle in this scope.
        ViewportChange viewport_change;
        reader_ptr->WaitForInitialMetadata();
        while (reader_ptr->Read(&viewport_change)) {
            auto const viewport_id = viewport_change.viewport_id().id();
            const std::lock_guard<std::mutex> write_lock(write_mutex);
            if (viewport_change.has_serial()) {
                DBG(CLOG << LOCATION << "viewport id: " << viewport_id << ", event kind: "
                         << viewport_change.viewport_change_kind() << " change serial: " << viewport_change.serial()
                         << ", data: [" << GetViewportData(viewport_id) << "]" << std::endl;);
            } else {
                DBG(CLOG << LOCATION << "viewport id: " << viewport_id
                         << ", event kind: " << viewport_change.viewport_change_kind() << std::endl;);
                if (omega_edit::ViewportChangeKind::VIEWPORT_EVT_CREATE == viewport_change.viewport_change_kind()) {
                    DBG(CLOG << LOCATION << "viewport id: " << viewport_id << " finishing" << std::endl;);
                    reader_ptr->Finish();
                    break;
                }
            }
        }
    }

    std::thread::id SubscribeOnChangeViewport(const std::string &viewport_id) {
        assert(!viewport_id.empty());
        ObjectId request;
        auto context_ptr = std::make_unique<ClientContext>();

        request.set_id(viewport_id);

        ViewportChange viewport_change;
        viewport_subscription_handler_thread_ = std::make_unique<std::thread>(
                std::thread(&OmegaEditServiceClient::HandleViewportChanges, this, std::move(context_ptr),
                            std::move(stub_->SubscribeOnChangeViewport(context_ptr.get(), request))));
        return viewport_subscription_handler_thread_->get_id();
    }

private:
    std::unique_ptr<Editor::Stub> stub_;
    std::unique_ptr<std::thread> viewport_subscription_handler_thread_ = nullptr;
};

int main(int argc, char **argv) {
    // Instantiate the client. It requires a channel, out of which the actual RPCs are created. This channel models a
    // connection to an endpoint specified by the argument "--target=" which is the only expected argument.
    // We indicate that the channel isn't authenticated (use of InsecureChannelCredentials()).
    std::string target_str = "localhost:50042";
    if (argc > 1) {
        const std::string arg_val = argv[1];
        const std::string arg_str("--target");
        size_t start_pos = arg_val.find(arg_str);
        if (start_pos != std::string::npos) {
            start_pos += arg_str.size();
            if (arg_val[start_pos] == '=') {
                target_str = arg_val.substr(start_pos + 1);
            } else {
                std::cout << "The only correct argument syntax is --target=" << std::endl;
                return EXIT_FAILURE;
            }
        } else {
            std::cout << "The only acceptable argument is --target=" << std::endl;
            return EXIT_FAILURE;
        }
    }
    DBG(CLOG << LOCATION << "Establishing a channel to Ωedit server on: " << target_str << std::endl;);
    OmegaEditServiceClient server_test_client(grpc::CreateChannel(target_str, grpc::InsecureChannelCredentials()));

    auto reply = server_test_client.GetOmegaEditVersion();
    DBG(CLOG << LOCATION << "Channel established to Ωedit server version: " << reply << " on " << target_str
             << std::endl;);

    auto session_id = server_test_client.CreateSession();
    DBG(CLOG << LOCATION << "CreateSession received: " << session_id << std::endl;);

    auto viewport_id = server_test_client.CreateViewport(session_id, 0, 100);
    {
        const std::lock_guard<std::mutex> write_lock(write_mutex);
        DBG(CLOG << LOCATION << "CreateViewport received: " << viewport_id << std::endl;);
    }

    auto thread_id = server_test_client.SubscribeOnChangeViewport(viewport_id);
    {
        const std::lock_guard<std::mutex> write_lock(write_mutex);
        DBG(CLOG << LOCATION << "SubscribeOnChangeViewport received: " << thread_id << std::endl;);
    }

    auto serial = server_test_client.Insert(session_id, 0, "Hello Weird!!!!");
    {
        const std::lock_guard<std::mutex> write_lock(write_mutex);
        DBG(CLOG << LOCATION << "Insert received: " << serial << std::endl;);
    }

    serial = server_test_client.Overwrite(session_id, 7, "orl");
    {
        const std::lock_guard<std::mutex> write_lock(write_mutex);
        DBG(CLOG << LOCATION << "Overwrite received: " << serial << std::endl;);
    }

    serial = server_test_client.Delete(session_id, 11, 3);
    {
        const std::lock_guard<std::mutex> write_lock(write_mutex);
        DBG(CLOG << LOCATION << "Delete received: " << serial << std::endl;);
    }

    reply = server_test_client.SaveSession(session_id, "/tmp/hello.txt");
    {
        const std::lock_guard<std::mutex> write_lock(write_mutex);
        DBG(CLOG << LOCATION << "SaveSession received: " << reply << std::endl;);
    }

    reply = server_test_client.GetViewportData(viewport_id);
    {
        const std::lock_guard<std::mutex> write_lock(write_mutex);
        DBG(CLOG << LOCATION << "Viewport data: [" << reply << "]" << std::endl;);
    }

    reply = server_test_client.DestroyViewport(viewport_id);
    {
        const std::lock_guard<std::mutex> write_lock(write_mutex);
        DBG(CLOG << LOCATION << "DestroyViewport received: " << reply << std::endl;);
    }

    reply = server_test_client.DestroySession(session_id);
    {
        const std::lock_guard<std::mutex> write_lock(write_mutex);
        DBG(CLOG << LOCATION << "DestroySession received: " << reply << std::endl;);
    }

    return EXIT_SUCCESS;
}