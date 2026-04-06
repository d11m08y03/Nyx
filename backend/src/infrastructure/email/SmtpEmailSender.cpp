#include "infrastructure/email/SmtpEmailSender.hpp"

#include <curl/curl.h>
#include <spdlog/spdlog.h>

namespace {
  struct UploadContext {
    std::string data;
    size_t bytes_read = 0;
  };

  auto read_callback(
    char* buffer, size_t size,
    size_t nitems, void* userdata
  ) -> size_t {
    auto* context =
      static_cast<UploadContext*>(userdata);
    auto remaining =
      context->data.size() - context->bytes_read;

    if (remaining == 0) {
      return 0;
    }

    auto bytes_to_copy =
      std::min(size * nitems, remaining);
    std::memcpy(
      buffer,
      context->data.data() + context->bytes_read,
      bytes_to_copy
    );
    context->bytes_read += bytes_to_copy;
    return bytes_to_copy;
  }
} // anonymous namespace

namespace Nyx::Infrastructure::Email {
  SmtpEmailSender::SmtpEmailSender(
    std::shared_ptr<
      Nyx::Infrastructure::Config::EnvironmentConfig
    > config
  ) : config(std::move(config)) {}

  auto SmtpEmailSender::send_verification_email(
    const std::string& to_email,
    const std::string& verification_token
  ) -> Nyx::Core::Result<void> {
    auto verification_url =
      this->config->frontend_url()
      + "/verify-email?token=" + verification_token;

    auto from_email = this->config->smtp_from_email();
    auto subject = std::string{"Verify your Nyx account"};

    auto email_body = std::string{
      "From: " + from_email + "\r\n"
      "To: " + to_email + "\r\n"
      "Subject: " + subject + "\r\n"
      "Content-Type: text/html; charset=UTF-8\r\n"
      "\r\n"
      "<html><body>"
      "<h2>Welcome to Nyx</h2>"
      "<p>Please verify your email address by "
      "clicking the link below:</p>"
      "<p><a href=\"" + verification_url + "\">"
      "Verify Email</a></p>"
      "<p>Or copy this URL into your browser:</p>"
      "<p>" + verification_url + "</p>"
      "<p>This link expires in 24 hours.</p>"
      "</body></html>"
    };

    auto context = UploadContext{
      .data = std::move(email_body)
    };

    auto* curl = curl_easy_init();
    if (curl == nullptr) {
      spdlog::error("Failed to initialize libcurl");
      return std::unexpected(
        Nyx::Core::AppError::internal(
          "Failed to send verification email"
        )
      );
    }

    auto smtp_url = std::string{
      (this->config->smtp_use_tls() ? "smtps://" : "smtp://")
      + this->config->smtp_host() + ":"
      + std::to_string(this->config->smtp_port())
    };

    curl_easy_setopt(curl, CURLOPT_URL, smtp_url.c_str());
    curl_easy_setopt(
      curl, CURLOPT_USERNAME,
      this->config->smtp_username().c_str()
    );
    curl_easy_setopt(
      curl, CURLOPT_PASSWORD,
      this->config->smtp_password().c_str()
    );
    curl_easy_setopt(
      curl, CURLOPT_MAIL_FROM, from_email.c_str()
    );

    struct curl_slist* recipients = nullptr;
    recipients =
      curl_slist_append(recipients, to_email.c_str());
    curl_easy_setopt(
      curl, CURLOPT_MAIL_RCPT, recipients
    );

    curl_easy_setopt(
      curl, CURLOPT_READFUNCTION, read_callback
    );
    curl_easy_setopt(
      curl, CURLOPT_READDATA, &context
    );
    curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);

    if (this->config->smtp_use_tls()) {
      curl_easy_setopt(
        curl, CURLOPT_USE_SSL, CURLUSESSL_ALL
      );
    }

    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);

    auto result = curl_easy_perform(curl);

    curl_slist_free_all(recipients);
    curl_easy_cleanup(curl);

    if (result != CURLE_OK) {
      spdlog::error(
        "SMTP send failed for {}: {}",
        to_email, curl_easy_strerror(result)
      );
      return std::unexpected(
        Nyx::Core::AppError::internal(
          "Failed to send verification email"
        )
      );
    }

    spdlog::info(
      "Verification email sent to {}", to_email
    );
    return {};
  }
} // namespace Nyx::Infrastructure::Email
