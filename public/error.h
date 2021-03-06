// Copyright 2016 The Fuchsia Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef VAPIDSSL_PUBLIC_ERROR_H
#define VAPIDSSL_PUBLIC_ERROR_H
#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

#include <stddef.h>
#include <stdint.h>

// This is the public API for error handling in VapidSSL.  It has API calls to
// get the most recent error and associated location information about where the
// error occurred.
//
// VapidSSL does not dynamically allocate any memory, so the API has routines to
// get the size of opaque structures, and API calls that need memory take it as
// an argument.

// tls_result_t is used to indicate whether or not a particular call executed
// successfully. All routines follow these rules with regards to return values:
// - Simple getters that cannot fail return primitives, e.g. size_t.
// - Finders and factories return pointers or NULL on error.
// - Functions which may fail due to network or API inputs return |kTlsFailure|
//   on error, |kTlsSuccess| otherwise.
// - All other functions return void.
// Note that 0 for return values
// indicates failure and thus make return values useful for conditional
// expressions and consistent with BoringSSL. Conversely, 0 for error codes
// indicates success, as with errno.
typedef enum tls_result_t {
  kTlsFailure = 0,
  kTlsSuccess = 1,
} tls_result_t;

// error_source_t indicates where an error occurred.  The error "reasons" below
// are |int|s whose meaning depends on which source generated the error.
// Different sources may reuse the same numeric value for different meanings,
// e.g. "22" may mean EINVAL if the corresponding source is |kTlsErrPlatform|,
// or a TLS alert of |kTlsErrRecordOverflow| if the source is |kTlsErrPeer| or
// |kTlsErrVapid|.
typedef enum tls_error_source_t {
  // Errors reported by the C standard library or underlying OS.  For these
  // errors, the reason is platform-specific.  For example, on Linux it may be
  // an |errno| defined in asm-generic/errno-base.h.
  kTlsErrPlatform,
  // Errors reported by the crypto library.  The reason is library-specific.  As
  // an example, for OpenSSL and BoringSSL the reason is the packed |int|
  // suitable for passing to the |ERR_GET_*| macros defined in openssl/err.h. */
  kTlsErrCrypto,
  // Errors received as TLS alerts from the remote peer. For these errors, the
  // reason is always a value of |tls_alert_t|.
  kTlsErrPeer,
  // Errors from the VapidSSL library itself. For these errors, the reason is
  // either a value of |tls_alert_t| or of |tls_error_t|.
  kTlsErrVapid,
} tls_error_source_t;

// These errors are the TLS alerts specified by
// https://tools.ietf.org/html/rfc5246#section-7.2.   These correspond to the
// |SSL3_AD_*| and |TLS1_AD_*| definitions in openssl/ssl.h and openssl/tls1.h,
// respectively.  That is, kTlsErrCloseNotify has the same value as
// SSL3_AD_CLOSE_NOTIFY, kTlsErrDecodeError has the same value as
// TLS1_AD_DECODE_TLS_ERROR, etc. These errors may either be received from a
// remote peer, or generated by the library causing an alert to be sent to the
// peer. */
typedef enum tls_alert_t {

  // Reporter received a CloseNotify.
  kTlsErrCloseNotify = 0,
  // Reporter received a message out of order or an unknown message.
  kTlsErrUnexpectedMessage = 10,
  // Reporter failed to decrypt and verify a record.
  kTlsErrBadRecordMac = 20,
  // Reporter received a message larger than its maximum fragment length.
  kTlsErrRecordOverflow = 22,
  // Reporter was unable to negotiate at least one security parameter with its
  // peer.
  kTlsErrHandshakeFailure = 40,
  // Reporter failed to decode a certificate or verify its signature.
  kTlsErrBadCertificate = 42,
  // Reporter received a certificate with an unsupported type.
  kTlsErrUnsupportedCertificate = 43,
  // Reporter received a revoked certificate.
  kTlsErrRevokedCertificate = 44,
  // Reporter received a revoked certificate.
  kTlsErrExpiredCertificate = 45,
  // Reporter encountered an unspecified error while parsing a received
  // certificate.
  kTlsErrCertificateUnknown = 46,
  // Reporter received an invalid parameter or one different than what was
  // specified earlier.
  kTlsErrIllegalParameter = 47,
  // Reporter received a certificate trust that does not include any
  // certificates matching the trusted certificate digests.
  kTlsErrUnknownCA = 48,
  // Reporter could not decode a message, either due to a bad length or an
  // unrecognized value for an enumerated field.
  kTlsErrDecodeError = 50,
  // Reporter failed to verify cryptographically sensitive data, namely either
  // the signature on key exchange parameters or the Finished message.
  kTlsErrDecryptError = 51,
  // Reporter received an unsupported version.
  kTlsErrProtocolVersion = 70,
  // Server requires stronger ciphers than the library supports.
  kTlsErrInsufficientSecurity = 71,
  // Reporter ran out of memory or encountered some other internal error.
  kTlsErrInternalError = 80,
  // Server sent a HelloRequest, which the library doesn't support.
  kTlsErrNoRenegotiation = 100,
  // Server sent a duplicate or unrequested extension.
  kTlsErrUnsupportedExtension = 110,
} tls_alert_t;

// These errors are the result of either invalid data from the remote peer or an
// application using the library's API incorrectly (e.g. not providing
// sufficient memory). They are generated by the VapidSSL library itself, i.e.
// |tls_error_source_t| is |kTlsErrVapid|.  The library may also use
// |tls_alert_t| values as reasons, thus this enum starts beyond the allowable
// range of alerts. If the library is connected, it may send an alert listed as
// described in each item below.
enum tls_error_t {
  // The remote peer sent an alert we didn't understand.  This error does not
  // send an alert because we don't want to send an alert in response to a bad
  // alert.
  kTlsErrBadAlert = 256,
  // The remote peer has sent an excessive number of empty fragments.  There
  // shouldn't be a strong need for empty fragments (no TLS 1.0 or CBC), and
  // excessive empty fragments becomes a DoS risk. This error sends an
  // |kTlsErrUnexpectedMessage| to the peer if connected.
  kTlsErrTooManyEmptyChunks,
  // The remote peer has sent an excessive number of non-fatal errors.
  kTlsErrTooManyWarnings,
  // The remote peer has sent an structure whose inner field lengths do not line
  // up with outer field lengths.  This error sends an |kTlsErrDecodeError| to
  // the peer if connected.
  kTlsErrLengthMismatch,
  // The remote peer closed the underlying transport in an orderly fashion
  // without shutting down the TLS connection.  This error does not send an
  // alert because the connection is gone!
  kTlsErrDisconnected,
  // The library needed more memory than what it was given. This error sends a
  // |kTlsErrInternalError| to the peer if connected.
  kTlsErrOutOfMemory,
  // The library detected that a fixed size counter overflowed. This error sends
  // a |kTlsErrInternalError| to the peer if connected.
  kTlsErrIntegerOverflow,
  // The library encountered a length mismatch when trying to read data from or
  // write data to one of its internal memory buffer. This error sends a
  // |kTlsErrInternalError| to the peer if connected.
  kTlsErrOutOfBounds,
  // The caller invoked an API call with null or incorrect arguments. This error
  // does not send an alert as it is only involves the calling application.
  kTlsErrInvalidArgument,
  // The caller invoked an API call at an inappropriate time, i.e.
  // calling|TLS_read| before |TLS_connect|. This error does not send an alert
  // as it is only involves the calling application.
  kTlsErrInvalidState,
  // Although VapidSSL knows about an algorithm, it isn't available in the
  // crypto library. This error sends a |kTlsErrHandshakeFailure| to the peer if
  // connected.
  kTlsErrUnsupportedAlgorithm,
  // The library's configuration does not include an acceptable set of security
  // parameters, e.g. all ciphers are disabled. This error does not send an
  // alert since it can only occur before connecting.
  kTlsErrNoAvailableOptions,
  // A buffer passed to TLS_read or TLS_write doesn't match the one passed to a
  // previous, incomplete call.  This error does not send an alert as it is only
  // involves the calling application.
  kTlsErrBufferChanged,
  // An unimplemented code path was followed. Should not occur outside
  // development! This error sends a |kTlsErrInternalError| to the peer if
  // connected.
  kTlsErrNotImplemented,
};

// TLS_ERROR_size sets |out| to be the amount of memory needed for a call to
// |TLS_ERROR_init| to create a thread-local error structure that can hold the
// packed error code and location information.
size_t TLS_ERROR_size(void);

// TLS_ERROR_init takes a memory region of |len| bytes starting at |mem| and
// uses it to create a thread-local error structure for the packed error code
// and location information.
tls_result_t TLS_ERROR_init(void *mem, size_t len);

// TLS_ERROR_get examines and copies the details of the the most recent error.
// If |TLS_ERROR_init| has not been called, it returns |kTlsFailure| with no
// effects.  Otherwise, it fills whichever of |out_source|, |out_reason|,
// |out_file| and |out_line| is not NULL and returns |kTlsSuccess|.  The meaning
// of the |out_reason| value depends on the value of |out_source|; see
// |tls_error_source_t| for details.  If no error was generated during the last
// API call, all of these fields will be 0 or NULL, as appropriate.
tls_result_t TLS_ERROR_get(tls_error_source_t *out_source, int *out_reason,
                           const char **out_file, int *out_line);

// TLS_ERROR_test checks whether the most recent error has a given |source| and
// |reason|.  More precisely, it returns |kTlsSuccess| if |TLS_ERROR_get| would
// set |out_source| to |source| and |out_reason| to |reason|, and |kTlsFailure|
// otherwise.
tls_result_t TLS_ERROR_test(tls_error_source_t source, int reason);

// TLS_ERROR_cleanup clears the thread-local error structure and returns the
// memory it was using for it.  Subsequent calls |TLS_ERROR_cleanup| do nothing
// and return NULL.
void *TLS_ERROR_cleanup(void);

#ifdef __cplusplus
}
#endif  // __cplusplus
#endif  // VAPIDSSL_PUBLIC_ERROR_H
