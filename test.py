import threading
import time
from concurrent.futures import ThreadPoolExecutor

import dns.resolver


def send_dns_query(resolver, domain):
    try:
        resolver.resolve(domain, 'A')
    except Exception as e:
        print(f"Query failed: {e}")


def load_test(target_ip, target_port, queries_per_second, duration, domains):
    resolver = dns.resolver.Resolver(configure=False)
    resolver.nameservers = [target_ip]
    resolver.port = target_port

    end_time = time.time() + duration

    with ThreadPoolExecutor(max_workers=100) as executor:
        while time.time() < end_time:
            start = time.time()
            futures = []
            for _ in range(queries_per_second):
                for domain in domains:
                    futures.append(executor.submit(send_dns_query, resolver, domain))

            for future in futures:
                future.result()

            elapsed = time.time() - start
            if elapsed < 1:
                time.sleep(1 - elapsed)


if __name__ == "__main__":
    target_ip = "127.0.0.1"  # Replace with your proxy's IP
    target_port = 53  # Replace with your proxy's port
    queries_per_second = 10000
    duration = 60  # seconds
    domains = [
        "microsoft.com",
        "google.com",
        "github.com",
    ]  # Add more domains as needed

    load_test(target_ip, target_port, queries_per_second, duration, domains)
